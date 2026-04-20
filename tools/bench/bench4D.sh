#!/usr/bin/env bash

# shellcheck disable=SC1091

set -eou pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT_DIR="${ROOT_DIR%/tools*}"
. "$ROOT_DIR/tools/utils/import.sh"
import utils/common_func.sh
import utils/build_utils.sh

log_info "Running 2D benchmarks..."

# TODO: Get the build path

# REMOVE AFTER TESTING
bash $ROOT_DIR/tools/build/partial_build.sh
if [ $? -ne 0 ]; then
    log_error "Build failed"
    exit 1
fi

CMAKE_BUILD_DIR="$ROOT_DIR/cmake-build-gnu-11-RelWithDebInfo-nosan"

# Parquet test file column name options
# "v_x":0.90132434,"v_y":0,"v_z":0,"rho":0.26100351,"e":2.8236782,"x":2.6768835,"y":0,"z":0}

"$CMAKE_BUILD_DIR/airtree-bench/airtree_bench" generate parquet \
    --input /home/rjairaj/.airtree/test_data/xx_36785.parquet \
    --schema "4DxF" \
    --columns "v_x" "v_y" "v_z" "rho"
if [ $? -ne 0 ]; then
    log_error "Benchmark failed"
    exit 1
fi

"$CMAKE_BUILD_DIR/airtree-bench/airtree_bench" generate parquet \
    --input /home/rjairaj/.airtree/test_data/xx_36785.parquet \
    --schema "4DxP" \
    --columns "v_x" "v_y" "v_z" "rho"
if [ $? -ne 0 ]; then
    log_error "Benchmark failed"
    exit 1
fi
