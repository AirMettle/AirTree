#!/usr/bin/env bash

set -eou pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT_DIR="${ROOT_DIR%/tools*}"
. "$ROOT_DIR/tools/utils/import.sh"
import utils/common_func.sh
import utils/build_utils.sh

log_info "Running 1D benchmarks..."

bash $ROOT_DIR/tools/build/partial_build.sh
if [ $? -ne 0 ]; then
    log_error "Build failed"
    exit 1
fi

SCRIPT_DIR="$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

BENCH_DATA_DIR="$CMAKE_BUILD_DIR/bench_data"
DATE_FOLDER="$(date +%d-%m-%Y)"
TIMESTAMP=$(date +%H:%M)
OUTPUT_DIR="$BENCH_DATA_DIR"/output/$DATE_FOLDER/benchmark-${TIMESTAMP}
BENCH_REPS="${BENCH_REPS:-5}"

if [ ! -d "$BENCH_DATA_DIR" ]; then
    log_warn "Benchmark data directory $BENCH_DATA_DIR does not exist. Fetching benchmark data..."
    bash "$ROOT_DIR/tools/bench/get_bench_datasets.sh" "$BENCH_DATA_DIR"
    if [ $? -ne 0 ]; then
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
            

            "$CMAKE_BUILD_DIR/airtree-bench/airtree_bench" --benchmark_out="$output_csv" \
            --benchmark_out_format=csv --benchmark_repetitions="$BENCH_REPS" \
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
     
            "$CMAKE_BUILD_DIR/airtree-bench/airtree_bench" --benchmark_out="$output_csv" \
            --benchmark_out_format=csv --benchmark_repetitions="$BENCH_REPS" \
            generate parquet \
            --input "$file" \
            --schema "$schema" \
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

# yellow 1D — TopK, MinMax, Percentile
for schema in 1DxF 1DxP; do
    hist="$AIRTREE_DIR/yellow_tripdata_${schema}.airtree"
    if [ ! -f "$hist" ]; then
        log_error "missing $hist"
        exit 1
    fi
    query_csv="$OUTPUT_DIR/query_yellow_tripdata_${schema}.csv"
    log_info "Query phase: $hist -> $query_csv"
    "$CMAKE_BUILD_DIR/airtree-bench/airtree_bench" \
        --benchmark_out="$query_csv" --benchmark_out_format=csv \
        query --input "$hist" --schema "$schema" \
        --queries topk minmax percentile
    if [ $? -ne 0 ]; then
        log_error "Query benchmark failed for $hist"
        exit 1
    fi
done

# yellow 2D/3D Precise — Grid + BoundingBox
for schema in 2DxP 3DxP; do
    hist="$AIRTREE_DIR/yellow_tripdata_${schema}.airtree"
    if [ ! -f "$hist" ]; then
        log_error "missing $hist"
        exit 1
    fi
    query_csv="$OUTPUT_DIR/query_yellow_tripdata_${schema}.csv"
    log_info "Query phase: $hist -> $query_csv"
    "$CMAKE_BUILD_DIR/airtree-bench/airtree_bench" \
        --benchmark_out="$query_csv" --benchmark_out_format=csv \
        query --input "$hist" --schema "$schema" \
        --queries grid boundingbox
    if [ $? -ne 0 ]; then
        log_error "Query benchmark failed for $hist"
        exit 1
    fi
done

# yellow 4D Precise — Grid only
hist="$AIRTREE_DIR/yellow_tripdata_4DxP.airtree"
if [ ! -f "$hist" ]; then
    log_error "missing $hist"
    exit 1
fi
query_csv="$OUTPUT_DIR/query_yellow_tripdata_4DxP.csv"
log_info "Query phase: $hist -> $query_csv"
"$CMAKE_BUILD_DIR/airtree-bench/airtree_bench" \
    --benchmark_out="$query_csv" --benchmark_out_format=csv \
    query --input "$hist" --schema 4DxP \
    --queries grid
if [ $? -ne 0 ]; then
    log_error "Query benchmark failed for $hist"
    exit 1
fi

# jane_street 1DxT
hist="$AIRTREE_DIR/jane_street_1DxT.airtree"
if [ ! -f "$hist" ]; then
    log_error "missing $hist"
    exit 1
fi
query_csv="$OUTPUT_DIR/query_jane_street_1DxT.csv"
log_info "Query phase: $hist -> $query_csv"
"$CMAKE_BUILD_DIR/airtree-bench/airtree_bench" \
    --benchmark_out="$query_csv" --benchmark_out_format=csv \
    query --input "$hist" --schema 1DxT \
    --queries topk minmax percentile
if [ $? -ne 0 ]; then
    log_error "Query benchmark failed for $hist"
    exit 1
fi

echo "Query benchmarks completed."

echo "consolidating benchmark statistics in consolidated_<schema>_data.csv"
bash "$SCRIPT_DIR"/consolidate_bench_data.sh "$OUTPUT_DIR"

echo "consolidating query benchmark statistics in consolidated_query_<schema>_data.csv"
bash "$SCRIPT_DIR"/consolidate_query_bench_data.sh "$OUTPUT_DIR"

echo "consolidating system info in system_info.csv"
bash "$SCRIPT_DIR"/consolidate_systeminfo.sh "$OUTPUT_DIR"

echo "Benchmark files saved in $OUTPUT_DIR"