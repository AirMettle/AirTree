#!/usr/bin/env bash

set -eou pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd )"
ROOT_DIR="${SCRIPT_DIR%/tools*}"
cd "$SCRIPT_DIR"

. "$ROOT_DIR/tools/utils/import.sh"
import utils/common_func.sh
import utils/build_utils.sh

log_info "Ensuring host toolchain via tools/setup/setup.sh"
if ! bash "$ROOT_DIR/tools/setup/setup.sh"; then
    log_error "Host setup failed"
    exit 1
fi

log_info "Running 1D benchmarks..."

if ! bash "$ROOT_DIR/tools/build/partial_build.sh"; then
    log_error "Build failed"
    exit 1
fi

BENCH_DATA_DIR="$CMAKE_BUILD_DIR/bench_data"
DATE_FOLDER="$(date +%d-%m-%Y)"
TIMESTAMP=$(date +%H:%M)
OUTPUT_DIR="$BENCH_DATA_DIR"/output/$DATE_FOLDER/benchmark-${TIMESTAMP}

if [ ! -d "$BENCH_DATA_DIR" ]; then
    log_warn "Benchmark data directory $BENCH_DATA_DIR does not exist. Fetching benchmark data..."
    if ! bash "$SCRIPT_DIR/get_bench_datasets.sh" "$BENCH_DATA_DIR"; then
        log_error "Failed to fetch benchmark data"
        exit 1
    fi
    log_info "Benchmark data fetched successfully."
else
    log_info "Benchmark data directory $BENCH_DATA_DIR already exists. Skipping download."
fi

mkdir -p "$OUTPUT_DIR"

mkdir -p "$OUTPUT_DIR/airtree_files"
AIRTREE_DIR="$OUTPUT_DIR/airtree_files"

# Pin to CPU 0 on Linux for stable benches. macOS/Windows have no taskset;
# run the binary directly so those hosts do not fail at generate/query.
if command -v taskset >/dev/null 2>&1; then
    PIN=(taskset -c 0)
else
    log_warn "taskset not found; running airtree_bench unpinned"
    PIN=()
fi
run_bench() {
    "${PIN[@]}" "$@"
}

parquet_schemas=( "1DxF" "1DxP" "2DxF" "2DxP" "3DxF" "3DxP" "4DxF" "4DxP")
binary_schemas=( "1DxT" "1DxF" "1DxP")

for schema in "${binary_schemas[@]}"; do
    
    dim="${schema:0:1}"

    echo "Starting benchmark generation for all .bin files..."

    for file in "$BENCH_DATA_DIR"/*.bin; do
        if [ -f "$file" ]; then
            filename=$(basename "$file")
            dataset_name="${filename%.bin}"
            output_csv="${OUTPUT_DIR}/${dataset_name}_${schema}.csv"
        
            echo "=================================================="
            echo "Processing: $filename"
        
            # Log the current configuration
            log_info "Running $schema ($dim dimensions) with columns: ${active_columns[*]}"
            log_info "Writing results to: $output_csv"
        
            
            write_args=()
            if [[ "$dataset_name" == "jane_street" && "$schema" == "1DxT" ]]; then
                write_args=(--write-airtree "$AIRTREE_DIR/jane_street_1DxT.airtree")
            fi
            

            run_bench "$CMAKE_BUILD_DIR/airtree-bench/airtree_bench" --benchmark_out="$output_csv" \
            --benchmark_out_format=csv \
            generate binary \
            --input "$file" \
            --schema "$schema" \
            --data-type float \
            "${write_args[@]}"
        
            # Check if command failed
            if [ $? -ne 0 ]; then
            log_error "Benchmark failed for schema $schema on file $filename"
            exit 1
            fi
        
            log_info "Benchmark for schema $schema completed successfully: $output_csv"
        fi

    done
done

#correlated columns from FRED-MD
all_MD_columns=( "RPI" "W875RX1" "RETAILx" "INDPRO" )

#correlated columns from FRED-QD
all_QD_columns=( "GDPC1" "DPIC96" "PCECC96" "OUTNFB")

#correlated columns from yellow_tripdata
all_YT_columns=( "trip_distance" "fare_amount" "total_amount" "tip_amount" )

echo "Starting benchmark generation for all .parquet files..."
for schema in "${parquet_schemas[@]}"; do

    dim="${schema:0:1}" 

    for file in "$BENCH_DATA_DIR"/*.parquet; do
        if [ -f "$file" ]; then
            filename=$(basename "$file")
            dataset_name="${filename%.parquet}"
            output_csv="${OUTPUT_DIR}/${dataset_name}_${schema}.csv"
        
            echo "=================================================="
            echo "Processing: $filename"
            active_columns=()
            if [[ "$filename" == *MD* ]]; then
                active_columns=("${all_MD_columns[@]:0:dim}")
            
            elif [[ "$filename" == *QD* ]]; then
                active_columns=("${all_QD_columns[@]:0:dim}")
            
            elif [[ "$filename" == yellow_tripdata*combined.parquet ]]; then
                active_columns=("${all_YT_columns[@]:0:dim}")
            
            else
                log_error "No column mapping for $filename"
                continue
            fi
            
            if [ "${#active_columns[@]}" -ne "$dim" ]; then
                log_error "Expected $dim columns from $filename, got ${#active_columns[@]}"
                exit 1
            fi

            # Log the current configuration
            log_info "Running $schema ($dim dimensions) with columns: ${active_columns[*]}"
            log_info "Writing results to: $output_csv"

            write_args=()
            if [[ "$filename" == yellow_tripdata*combined.parquet ]]; then
                case "$schema" in
                    1DxF|1DxP|2DxP|3DxP|4DxP)
                        write_args=(--write-airtree "$AIRTREE_DIR/yellow_tripdata_${schema}.airtree")
                        ;;
                esac
            fi
     
            run_bench "$CMAKE_BUILD_DIR/airtree-bench/airtree_bench" --benchmark_out="$output_csv" \
            --benchmark_out_format=csv \
            generate parquet \
            --input "$file" \
            --schema "$schema" \
            --data-type float \
            --columns "${active_columns[@]}" \
            "${write_args[@]}"
        
            # Check if command failed
            if [ $? -ne 0 ]; then
            log_error "Benchmark failed for schema $schema on file $filename"
            exit 1
            fi
        
            log_info "Benchmark for schema $schema completed successfully: $output_csv"
        fi

    done

done

echo "Benchmarks completed."


echo "Starting query benchmarks on $AIRTREE_DIR ..."

run_query_suite() {
    local hist="$1"
    local schema="$2"
    local query_csv="$3"
    shift 3
    if [ ! -f "$hist" ]; then
        log_warn "Skipping query for schema $schema: missing $hist"
        return 0
    fi
    log_info "Query phase: $hist -> $query_csv"
    if ! run_bench "$CMAKE_BUILD_DIR/airtree-bench/airtree_bench" \
        --benchmark_out="$query_csv" --benchmark_out_format=csv \
        query --input "$hist" --schema "$schema" \
        --queries "$@"; then
        log_warn "Query benchmark failed for $hist (continuing)"
    fi
}

# yellow 1D — TopK, MinMax, Percentile
for schema in 1DxF 1DxP; do
    run_query_suite "$AIRTREE_DIR/yellow_tripdata_${schema}.airtree" "$schema" \
        "$OUTPUT_DIR/query_yellow_tripdata_${schema}.csv" topk minmax percentile
done

# yellow 2D/3D Precise — Grid + BoundingBox
for schema in 2DxP 3DxP; do
    run_query_suite "$AIRTREE_DIR/yellow_tripdata_${schema}.airtree" "$schema" \
        "$OUTPUT_DIR/query_yellow_tripdata_${schema}.csv" grid boundingbox
done

# yellow 4D Precise — Grid only
run_query_suite "$AIRTREE_DIR/yellow_tripdata_4DxP.airtree" 4DxP \
    "$OUTPUT_DIR/query_yellow_tripdata_4DxP.csv" grid

# jane_street 1DxT
run_query_suite "$AIRTREE_DIR/jane_street_1DxT.airtree" 1DxT \
    "$OUTPUT_DIR/query_jane_street_1DxT.csv" topk minmax percentile

echo "Query benchmarks completed."

echo "consolidating benchmark statistics in consolidated_<schema>_data.csv"
bash "$SCRIPT_DIR"/consolidate_bench_data.sh "$OUTPUT_DIR"

echo "consolidating query benchmark statistics in consolidated_query_<schema>_data.csv"
bash "$SCRIPT_DIR"/consolidate_query_bench_data.sh "$OUTPUT_DIR"

echo "consolidating system info in system_info.csv"
bash "$SCRIPT_DIR"/consolidate_systeminfo.sh "$OUTPUT_DIR"

echo "Generating plots in $OUTPUT_DIR/plots"
# Plot deps live in the project venv, not system python3. Create the venv
# and pip-install requirements-plot.txt when this machine has never plotted.
PLOT_REQ="$SCRIPT_DIR/requirements-plot.txt"
PLOT_PYTHON=""
if [[ ! -x "$CMAKE_BUILD_DIR/venv/bin/python" ]]; then
    log_info "No project venv at $CMAKE_BUILD_DIR/venv; creating via tools/setup/venv.sh"
    if ! bash "$ROOT_DIR/tools/setup/venv.sh"; then
        log_warn "Skipping plots: failed to create $CMAKE_BUILD_DIR/venv (bench CSVs are still valid)."
    fi
fi
if [[ -x "$CMAKE_BUILD_DIR/venv/bin/python" ]]; then
    PLOT_PYTHON="$CMAKE_BUILD_DIR/venv/bin/python"
    if ! "$PLOT_PYTHON" -c "import matplotlib, seaborn, pandas" >/dev/null 2>&1; then
        log_info "Installing plot deps into $CMAKE_BUILD_DIR/venv from $PLOT_REQ"
        if ! "$PLOT_PYTHON" -m pip install -r "$PLOT_REQ"; then
            log_warn "pip install of $PLOT_REQ failed; will skip plots if imports still fail."
            log_warn "Retry: $PLOT_PYTHON -m pip install -r $PLOT_REQ"
        fi
    fi
fi
if [[ -n "$PLOT_PYTHON" ]] && "$PLOT_PYTHON" -c "import matplotlib, seaborn, pandas" >/dev/null 2>&1; then
    log_info "Writing plots with $PLOT_PYTHON $SCRIPT_DIR/plot_benchmarks.py $OUTPUT_DIR --format png,svg"
    "$PLOT_PYTHON" "$SCRIPT_DIR/plot_benchmarks.py" "$OUTPUT_DIR" --format png,svg
    log_info "Plots written to $OUTPUT_DIR/plots"
elif [[ -n "$PLOT_PYTHON" ]]; then
    log_warn "Skipping plots: $PLOT_PYTHON still cannot import matplotlib, seaborn, and pandas (bench CSVs are still valid)."
    log_warn "Or plot later: $PLOT_PYTHON $SCRIPT_DIR/plot_benchmarks.py $OUTPUT_DIR --format png,svg"
else
    log_warn "Or plot later: python3 $SCRIPT_DIR/plot_benchmarks.py $OUTPUT_DIR --format png,svg"
fi

echo "Benchmark files saved in $OUTPUT_DIR"