#! /usr/bin/env bash

set -euo pipefail

# Triggers CPack packaging
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT_DIR="${ROOT_DIR%/tools*}"
. "$ROOT_DIR/tools/utils/import.sh"
import utils/build_utils.sh

# CPack - Package the application
echo "Packaging AirMettle AirTree with BUILD_TYPE=${BUILD_TYPE}"
# Package into platform specific package using cpack
cmake --build "${CMAKE_BUILD_DIR}" --target package
if [[ $? -ne 0 ]]
then
    echo "CMake package failed"
    exit 1
fi
