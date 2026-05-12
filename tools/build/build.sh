#!/usr/bin/env bash

# shellcheck disable=SC1091

set -eou pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT_DIR="${ROOT_DIR%/tools*}"
. "$ROOT_DIR/tools/utils/import.sh"
import utils/common_func.sh
import utils/build_utils.sh


# ---------------------------------------------------------------------------
# Usage
# ---------------------------------------------------------------------------
_bs_usage() {
    cat <<'HELP'
Usage: build.sh [OPTIONS] [COMMAND...]

Build orchestrator for the AirMettle AirTree project. Runs build stages sequentially.
With no commands, all stages run in order.

Commands:
  setup       Platform detection and OS-level setup
  airtree        Build C++ dependencies, configure & build the main project,
              run functional tests, and create DEB/RPM/TGZ packages
  clean       Remove all build artifacts (cmake-build-*, venvs, wheels, etc.)
  clean_deps  Remove all cached third-party dependencies (~/.airmettle/airtree-deps)

Options:
  -h, --help  Show this help message and exit

Examples:
  build.sh                  Run all stages
  build.sh setup            Run setup only
  build.sh airtree          Build C++ project (deps, venv, build, tests, package)
  build.sh setup airtree    Run setup then build C++ project
  build.sh clean            Remove all build artifacts
  build.sh clean_deps       Remove cached third-party dependencies

Notes:
  - Commands always execute in the fixed order: setup, airtree
    regardless of the order specified on the command line.
  - Dependencies between commands are NOT enforced. If you run 'airtree' without
    having previously run 'setup', the build may fail.
  - To build specific CMake targets, use partial_build.sh directly
    (run partial_build.sh --help for details).
HELP
}


# ---------------------------------------------------------------------------
# Parse arguments
# ---------------------------------------------------------------------------
_BS_RUN_SETUP=0
_BS_RUN_AIRTREE=0
_BS_RUN_CLEAN=0
_BS_RUN_CLEAN_DEPS=0
_BS_EXPLICIT=0

for arg in "$@"; do
    case "$arg" in
        -h|--help)
            _bs_usage
            exit 0
            ;;
        setup)
            _BS_RUN_SETUP=1
            _BS_EXPLICIT=1
            ;;
        airtree)
            _BS_RUN_AIRTREE=1
            _BS_EXPLICIT=1
            ;;
        clean)
            _BS_RUN_CLEAN=1
            _BS_EXPLICIT=1
            ;;
        clean_deps)
            _BS_RUN_CLEAN_DEPS=1
            _BS_EXPLICIT=1
            ;;
        *)
            log_error "Unknown command: $arg"
            _bs_usage >&2
            exit 1
            ;;
    esac
done

# No commands specified — run everything (preserves original default behaviour)
if [[ "$_BS_EXPLICIT" -eq 0 ]]; then
    _BS_RUN_SETUP=1
    _BS_RUN_AIRTREE=1
fi

_BS_ARCH=$(uname -m | awk '{ print tolower($0) }')


# ---------------------------------------------------------------------------
# clean — Remove all build artifacts
# ---------------------------------------------------------------------------
if [[ "$_BS_RUN_CLEAN" -eq 1 ]]; then
    log_info "Running stage: clean"

    # airtree build artifacts
    run_step --allow-fail "Removing cmake-build-* directories" \
        rm -rf "$PROJECT_ROOT"/cmake-build-*

    log_success "Clean complete."
    exit 0
fi


# ---------------------------------------------------------------------------
# clean_deps — Remove all cached third-party dependencies
# ---------------------------------------------------------------------------
if [[ "$_BS_RUN_CLEAN_DEPS" -eq 1 ]]; then
    log_info "Running stage: clean_deps"

    _BS_DEPS_DIR="$HOME/.airmettle/airtree-deps"

    if [[ -d "$_BS_DEPS_DIR" ]]; then
        run_step "Removing cached dependencies at $_BS_DEPS_DIR" \
            rm -rf "$_BS_DEPS_DIR"
    else
        log_info "No cached dependencies found at $_BS_DEPS_DIR"
    fi

    log_success "Dependency clean complete."
    exit 0
fi


# ---------------------------------------------------------------------------
# setup — Platform detection and OS-level setup
# ---------------------------------------------------------------------------
if [[ "$_BS_RUN_SETUP" -eq 1 ]]; then
    log_info "Running stage: setup"
    if ! . "$TOOLS_DIR/setup/setup.sh"; then
        error_exit "Setup failed"
    fi
fi

log_info "TOOLS_DIR: $TOOLS_DIR"


# ---------------------------------------------------------------------------
# airtree — Venv, build (deps included), functional tests, CPack packaging
# ---------------------------------------------------------------------------
if [[ "$_BS_RUN_AIRTREE" -eq 1 ]]; then
    log_info "Running stage: airtree"

    # Setting up venv
    if ! . "$TOOLS_DIR/setup/venv.sh"; then
        error_exit "Virtual environment setup failed"
    fi

    # Compile and build the main project
    if ! . "$TOOLS_DIR/build/partial_build.sh"; then
        error_exit "Partial build failed"
    fi

    # Run functional tests
    if ! . "$TOOLS_DIR/build/fn_tests.sh"; then
        error_exit "Functional tests failed"
    fi

    # Trigger CPack
    if [[ "$OSTYPE" == "msys"* || "$OSTYPE" == "cygwin"* || "$OSTYPE" == "darwin"* ]]; then
        log_warn "Packaging is not supported on Windows/macOS. Please use WSL or a Linux environment."
    elif ! . "$TOOLS_DIR/build/package.sh"; then
        error_exit "CPack packaging failed"
    fi
fi
