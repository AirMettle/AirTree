#!/usr/bin/env bash

# shellcheck disable=SC1091

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT_DIR="${ROOT_DIR%/tools*}"
. "$ROOT_DIR/tools/utils/import.sh"
import utils/common_func.sh
import utils/build_utils.sh

_CMAKE_SOURCE_DIR="$CMAKE_SOURCE_DIR"
_CMAKE_BUILD_DIR="$CMAKE_BUILD_DIR"

_pb_usage() {
    cat <<'HELP'
Usage: partial_build.sh [OPTIONS] [TARGET...]

Build one or more CMake targets. With no targets, builds everything (same as 'all').
CMake's dependency graph ensures prerequisites are built automatically.

Common targets:
  airtree-util, airtree-core, airtree-query, airtree-reader,
  libairtree-merge, libairtree-export,
  airtree, airtree-merge, airtree-export

Special targets:
  all         Build everything (default)

Options:
  -h, --help  Show this help message and exit
  --list      List all available CMake targets (requires a configured build)

Priority: CLI args > BUILD_TARGET env var > default (all)

Examples:
  partial_build.sh                        # Full build (all)
  partial_build.sh airtree-query             # Build airtree-query (+ deps)
  partial_build.sh airtree-query airtree    # Build multiple targets
  BUILD_TARGET=airtree partial_build.sh  # Via environment variable
HELP
}

_pb_list_targets() {
    if [[ -d "$_CMAKE_BUILD_DIR" ]]; then
        cmake --build "$_CMAKE_BUILD_DIR" --target help 2>/dev/null \
            | grep ': phony$' \
            | sed 's/: phony$//' \
            | grep -v '/' \
            | sort
    else
        log_warn "Build directory not found: $_CMAKE_BUILD_DIR"
        log_info "Run a full build first, or use --help to see common targets"
        exit 1
    fi
}

_PB_TARGETS=()

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    # Executed directly — parse our own $@
    for arg in "$@"; do
        case "$arg" in
            -h|--help)
                _pb_usage
                exit 0
                ;;
            --list)
                _pb_list_targets
                exit 0
                ;;
            *)
                _PB_TARGETS+=("$arg")
                ;;
        esac
    done
fi

# Fall back to BUILD_TARGET env var, then to "all"
if [[ ${#_PB_TARGETS[@]} -eq 0 ]]; then
    if [[ -v BUILD_TARGET && -n "$BUILD_TARGET" ]]; then
        _PB_TARGETS=("$BUILD_TARGET")
    else
        _PB_TARGETS=(all)
    fi
fi

# Determine whether this is a targeted build (skip unit tests) or a full build
_PB_FULL_BUILD=1
for t in "${_PB_TARGETS[@]}"; do
    if [[ "$t" != "all" ]]; then
        _PB_FULL_BUILD=0
        break
    fi
done

if [[ ! -d "$_CMAKE_BUILD_DIR" ]]; then
    run_step "Creating build directory" mkdir -p "$_CMAKE_BUILD_DIR"
fi

pushd $_CMAKE_BUILD_DIR >/dev/null

# determine parallelism
NUM_CORES=$(get_parallel_processes | tail -1)
log_debug "Using $NUM_CORES cores for parallel build"

log_info "Using PROJECT_ROOT=$PROJECT_ROOT"
log_info "Configuring and building in root directory"

# On Windows (TOOLCHAIN forced to msvc in settings.sh), keep MSVC tools first in PATH
OS_NAME="$(uname | awk '{ print tolower($0) }')"
if [[ "$OS_NAME" == *"mingw"* || "$OS_NAME" == *"msys"* || "$OS_NAME" == *"cygwin"* ]]; then
    if command -v cl.exe >/dev/null 2>&1; then
        MSVC_BIN_DIR="$(dirname "$(which cl.exe)")"
        export PATH="$MSVC_BIN_DIR:$PATH"
        log_info "Forced MSVC tools to the front of PATH"
    fi
fi

CMAKE_TOOLCHAIN_FILE="${_CMAKE_SOURCE_DIR}/cmake/Toolchain_${TOOLCHAIN//-/_}.cmake"

# Configure cmake
run_step "Configuring CMake project" \
    cmake -G "${GENERATOR}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN_FILE" \
    -DAIRTREE_SANITIZER="${SANITIZER}" \
    -DAIRTREE_LOG_LEVEL="${CPP_LOG_LEVEL}" \
    -DAIRTREE_DEPS_CACHE_ENABLED="${AIRTREE_DEPS_CACHE_ENABLED}" \
    -S "${_CMAKE_SOURCE_DIR}" \
    -B "${_CMAKE_BUILD_DIR}"

log_info "Current working directory: $(pwd)"

# Disabling this step since we rely on CMake's dependency graph to build prerequisites automatically when building requested targets.
# Build dependencies
# run_step "Building dependencies target (${NUM_CORES} cores)" \
#     cmake --build "${_CMAKE_BUILD_DIR}" \
#     --target "am_airtree_dependencies" \
#     --parallel "${NUM_CORES}"

# Build requested targets
for _pb_target in "${_PB_TARGETS[@]}"; do
    run_step "Building target: ${_pb_target} (${NUM_CORES} cores)" \
        cmake --build "${_CMAKE_BUILD_DIR}" \
        --target "${_pb_target}" \
        --parallel "${NUM_CORES}"
done

# Run unit tests only for full builds (temproray until unit tests are moved to its own modules)
if [[ "$_PB_FULL_BUILD" -eq 1 ]] && [[ -f "$PROJECT_ROOT/tools/build/unit_tests.sh" ]]; then
    run_step "Running unit tests" \
        bash "$PROJECT_ROOT/tools/build/unit_tests.sh"
fi

popd >/dev/null
