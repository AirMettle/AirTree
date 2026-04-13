#!/usr/bin/env bash
set -eou pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT_DIR="${ROOT_DIR%/tools*}"
. "$ROOT_DIR/tools/utils/import.sh"

import utils/build_utils.sh
import utils/common_func.sh
import utils/logger.sh
import utils/test_utils.sh

log_debug "CMAKE_SOURCE_DIR=$CMAKE_SOURCE_DIR"
log_debug "CMAKE_BUILD_DIR=$CMAKE_BUILD_DIR"

run_tests "UNIT" "^ut_" "$CMAKE_BUILD_DIR" || exit 1
