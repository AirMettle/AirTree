#!/usr/bin/env bash

# Script to benchmark AirTree histogram generation performance on NYC Taxi dataset.
# Data schema:
# {
#     "VendorID":2,
#     "tpep_pickup_datetime":1704070675000,
#     "tpep_dropoff_datetime":1704071863000,
#     "passenger_count":1,
#     "trip_distance":1.72,
#     "RatecodeID":1,
#     "store_and_fwd_flag":"N",
#     "PULocationID":186,
#     "DOLocationID":79,
#     "payment_type":2,
#     "fare_amount":17.7,
#     "extra":1,
#     "mta_tax":0.5,
#     "tip_amount":0,
#     "tolls_amount":0,
#     "improvement_surcharge":1,
#     "total_amount":22.7,
#     "congestion_surcharge":2.5,
#     "Airport_fee":0
# }

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

_CMAKE_BUILD_DIR="$CMAKE_BUILD_DIR"

BENCH_DATA_DIR="$_CMAKE_BUILD_DIR/bench_data"
if [ ! -d "$BENCH_DATA_DIR" ]; then
    log_warn "Benchmark data directory $BENCH_DATA_DIR does not exist. Fetching benchmark data..."
    bash "$ROOT_DIR/tools/bench/data.sh" "$BENCH_DATA_DIR"
    if [ $? -ne 0 ]; then
        log_error "Failed to fetch benchmark data"
        exit 1
    fi
    log_info "Benchmark data fetched successfully."
fi

schemas=("1DxT" "1DxF" "1DxP" "2DxF" "2DxP" "3DxF" "3DxP" "4DxF" "4DxP")
all_columns=("trip_distance" "fare_amount" "tip_amount" "congestion_surcharge")

for schema in "${schemas[@]}"; do
    dim="${schema:0:1}"

    active_columns=("${all_columns[@]:0:$dim}")

    log_info "Running $schema ($dim dimensions) with columns: ${active_columns[*]}"

    "$_CMAKE_BUILD_DIR/airtree-bench/airtree_bench" generate parquet \
        --input "$BENCH_DATA_DIR/yellow_tripdata_2024-01.parquet" \
        --schema "$schema" \
        --columns "${active_columns[@]}"
        
    if [ $? -ne 0 ]; then
        log_error "Benchmark failed for schema $schema"
        exit 1
    fi
    log_info "Benchmark for schema $schema completed successfully."
done
