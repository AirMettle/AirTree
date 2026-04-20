#! /usr/bin/env bash

set -eou pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT_DIR="${ROOT_DIR%/tools*}"
. "$ROOT_DIR/tools/utils/import.sh"
import utils/common_func.sh
import utils/build_utils.sh

if [ $# -ne 1 ]; then
    log_error "Usage: $0 <destination_directory>"
    exit 1
fi

log_info "Fetching data for benchmarks..."

BENCH_DATA_DIR="$1"
mkdir -p "$BENCH_DATA_DIR"

log_info "Benchmark data set will be store in $BENCH_DATA_DIR"

# FIXME: This is a small realistic dataset. We should probably look into running benchmarks against TPC-H SF1 or SF10 datasets in the future. 
log_info "Downloading yellow_tripdata_2024-01.parquet (NYC Taxi Data)..."
curl -o "$BENCH_DATA_DIR/yellow_tripdata_2024-01.parquet" "https://d37ci6vzurychx.cloudfront.net/trip-data/yellow_tripdata_2024-01.parquet"
if [ $? -ne 0 ]; then
    log_error "Failed to download yellow_tripdata_2024-01.parquet"
    exit 1
fi
