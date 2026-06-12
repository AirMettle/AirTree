#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT_DIR="${ROOT_DIR%/tools*}"
. "$ROOT_DIR/tools/utils/import.sh"

import utils/common_func.sh
import utils/build_utils.sh

OS_NAME="$(uname | awk '{ print tolower($0) }')"

SUDO=$(determine_sudo)
log_debug "Using sudo command: '${SUDO}'"

PYTHON_VENV_DIR="${CMAKE_BUILD_DIR}/venv"
# To handle cases where we don't want our CMake build directory but still want to use this script for venv setup
# we will create an empty CMake build directory if it doesn't exist, to serve as the root for the venv. 
# This is a bit hacky but allows us to reuse the same venv setup logic without needing to parameterize the venv location everywhere.
if [[ ! -d "${CMAKE_BUILD_DIR}" ]]; then
    log_info "Python venv root does not exist at ${PYTHON_VENV_DIR}. Creating..."
    mkdir -p "${CMAKE_BUILD_DIR}"
fi

# Avoid interactive prompts during apt operations
export DEBIAN_FRONTEND=noninteractive

log_info "Setting up Python environment..."

OS_NAME="$(uname | awk '{ print tolower($0) }')"

# Find system Python
find_system_python() {    
    if [[ "$OS_NAME" == *"mingw"* || "$OS_NAME" == *"msys"* || "$OS_NAME" == *"cygwin"* ]]; then
        if command -v python >/dev/null 2>&1; then
            command -v python
        elif command -v python3 >/dev/null 2>&1; then
            command -v python3
        else
            find /c/Python31* /c/tools/python31* "/c/Program Files/Python31*" -maxdepth 1 -name "python.exe" 2>/dev/null | head -1 || true
        fi
    elif [[ "$OS_NAME" == "darwin" ]]; then
        # BSD find lacks -executable; python@3.12 may be keg-only (not in PATH)
        if command -v python3.12 >/dev/null 2>&1; then
            command -v python3.12
        else
            local _brew_py
            _brew_py="$(brew --prefix python@3.12 2>/dev/null)/bin/python3.12"
            if [[ -x "$_brew_py" ]]; then echo "$_brew_py"; fi
        fi
    else
        find /bin /usr/bin /usr/local/bin -executable -name python3.12 2>/dev/null | head -1 || true
    fi
}

SYSTEM_PYTHON=$(find_system_python)
if [[ -z "$SYSTEM_PYTHON" ]]; then
    log_error "Python 3.12 not found in standard locations"
    exit 1
fi
log_info "Found system Python: $SYSTEM_PYTHON"

# Create virtual environment if needed
if [[ ! -x "${PYTHON_VENV_DIR}/bin/python3.12" ]]; then
    log_info "Creating virtual environment..."

    run_step "Removing existing venv directory" rm -rf "${PYTHON_VENV_DIR}"

    # Try stdlib venv first
    if run_step --allow-fail "Creating venv with stdlib venv" "${SYSTEM_PYTHON}" -m venv "${PYTHON_VENV_DIR}"; then
        log_success "Created virtual environment with stdlib venv"
    else
        log_warn "stdlib venv failed (likely ensurepip missing). Attempting OS package install..."

        if command -v apt >/dev/null 2>&1; then
            # Get Python version
            PYVER=$("${SYSTEM_PYTHON}" -c 'import sys; print(f"{sys.version_info.major}.{sys.version_info.minor}")')
            log_debug "Python version: $PYVER"

            run_step --allow-fail "Updating apt package list" ${SUDO} apt update

            # Try to install venv package
            if run_step --allow-fail "Installing python venv package" ${SUDO} apt install -y "python${PYVER}-venv"; then
                log_debug "Installed python${PYVER}-venv package"
            else
                run_step "Installing python3.12-venv package" ${SUDO} apt install -y python3.12-venv
            fi

            # Try venv again
            if run_step --allow-fail "Creating venv after package install" "${SYSTEM_PYTHON}" -m venv "${PYTHON_VENV_DIR}"; then
                log_success "Created virtual environment with stdlib venv (after installing OS package)"
            else
                log_warn "stdlib venv still failing; falling back to virtualenv..."
                create_venv_with_virtualenv
            fi
        else
            log_info "apt not available; falling back to virtualenv..."
            create_venv_with_virtualenv
        fi
    fi
fi

create_venv_with_virtualenv() {
    # Install virtualenv if not present
    if ! "${SYSTEM_PYTHON}" -m pip show virtualenv >/dev/null 2>&1; then
        run_step "Installing virtualenv" "${SYSTEM_PYTHON}" -m pip install --user virtualenv
    fi

    run_step "Creating venv with virtualenv" \
        env DEB_PYTHON_INSTALL_LAYOUT='deb' "${SYSTEM_PYTHON}" -m virtualenv "${PYTHON_VENV_DIR}"

    log_success "Created virtual environment with virtualenv"
}

# Activate venv
log_info "Activating virtual environment..."
if [[ "$OS_NAME" == *"mingw"* || "$OS_NAME" == *"msys"* || "$OS_NAME" == *"cygwin"* ]]; then
    source "${PYTHON_VENV_DIR}/Scripts/activate" || {
        log_error "Failed to activate virtual environment at ${PYTHON_VENV_DIR}"
        exit 1
    }
else
    source "${PYTHON_VENV_DIR}/bin/activate" || {
        log_error "Failed to activate virtual environment at ${PYTHON_VENV_DIR}"
        exit 1
    }
fi

VENV_PYTHON="$(python -c 'import sys; print(sys.executable)')"
log_info "Using venv Python: ${VENV_PYTHON}"
log_info "Python version: $(python -V)"

# Bootstrap pip if missing
if ! python -m pip --version >/dev/null 2>&1; then
    log_warn "pip missing in venv; bootstrapping with ensurepip..."
    run_step --allow-fail "Bootstrapping pip with ensurepip" python -m ensurepip --upgrade
fi

log_success "Python virtual environment setup complete."
# Installing pyyaml in the venv for version parsing
run_step "Running pip upgrade" python -m pip install --upgrade pip
run_step "Installing pyyaml in venv" python -m pip install pyyaml==6.0.2
