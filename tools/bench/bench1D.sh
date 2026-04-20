#!/usr/bin/env bash

# shellcheck disable=SC1091

set -eou pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT_DIR="${ROOT_DIR%/tools*}"
. "$ROOT_DIR/tools/utils/import.sh"
import utils/common_func.sh
import utils/build_utils.sh

log_info "Running 1D benchmarks..."

# TODO: Get the build path

# REMOVE AFTER TESTING
bash $ROOT_DIR/tools/build/partial_build.sh
if [ $? -ne 0 ]; then
    log_error "Build failed"
    exit 1
fi

CMAKE_BUILD_DIR="$ROOT_DIR/cmake-build-gnu-11-RelWithDebInfo-nosan"

"$CMAKE_BUILD_DIR/airtree-bench/airtree_bench" generate binary \
    --input ~/workspace/AirMettle/AirTree/tests/TestData/dim1_vx.bin \
    --schema "1DxF" \
    --data-type "double"
if [ $? -ne 0 ]; then
    log_error "Benchmark failed"
    exit 1
fi
