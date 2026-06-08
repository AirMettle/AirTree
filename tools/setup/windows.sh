#! /usr/bin/env bash

set -eou pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT_DIR="${ROOT_DIR%/tools*}"
. "$ROOT_DIR/tools/utils/import.sh"
import utils/common_func.sh

OS="$(uname | awk '{ print tolower($0) }')"
ARCH="$(uname -m | awk '{ print tolower($0) }')"

if [[ "$OS" != *"mingw"* && "$OS" != *"msys"* && "$OS" != *"cygwin"* ]]; then
  error_exit "This script is intended for Windows (MinGW/MSYS) architecture."
fi

log "INFO" "Installing dependencies on Windows ($OS) $ARCH ..."

if ! net session &>/dev/null; then
  error_exit "This script MUST be run as Administrator to install Chocolatey packages."
fi

if ! command -v choco >/dev/null 2>&1; then
  log "WARNING" "Chocolatey not found. Installing Chocolatey..."
  powershell -NoProfile -ExecutionPolicy Bypass -Command "iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))"
  export PATH="$PATH:/c/ProgramData/chocolatey/bin"
fi

if ! command -v cl.exe >/dev/null 2>&1; then
  error_exit "MSVC compiler (cl.exe) not found! Run this inside the 'x64 Native Tools Command Prompt'."
fi

# Helper function for Chocolatey
install_package() {
  local package=$1
  local version_flag=${2:-}
  run_step "[ airtree-setup ] Install $package" choco install "$package" $version_flag -y --no-progress
}

install_package "wget"
install_package "curl"
install_package "unzip"
# Can't install git via Chocolatey using git bash on windows since we are already running git bash :). I'm going to
# skip this and add it as a hard requirement for windows users to have git installed and in their PATH.
# install_package "git"
install_package "cppcheck"
install_package "ninja"
install_package "sccache"
install_package "winflexbison3"
install_package "pkgconfiglite"
install_package "strawberryperl"
install_package "nasm"

CMAKE_REQUIRED="3.27.9"
install_package "cmake" "--version $CMAKE_REQUIRED --force"
export PATH="/c/Program Files/CMake/bin:$PATH"

if command -v python >/dev/null 2>&1 || command -v py >/dev/null 2>&1; then
  log "INFO" "Python already installed on system. Skipping Chocolatey install."
else
  install_package "python" "--version 3.12.3"
fi

if command -v aws >/dev/null 2>&1; then
  log "INFO" "AWS CLI already installed: $(aws --version)"
else
  install_package "awscli"
fi

VCPKG_DIR="C:/vcpkg"
if [ ! -d "$VCPKG_DIR" ]; then
  log "INFO" "Setting up vcpkg at $VCPKG_DIR..."
  run_step "[ airtree-setup ] Clone vcpkg" git clone https://github.com/microsoft/vcpkg.git "$VCPKG_DIR"
  run_step "[ airtree-setup ] Bootstrap vcpkg" "$VCPKG_DIR/bootstrap-vcpkg.bat" -disableMetrics
fi

# Note: 'x64-windows' builds dynamically (.dll). Use 'x64-windows-static' for static versions of libs.
# Snappy is required by arrow and parquet and vcpkg will try to look for the dynamic version of snappy. I'm
# leaving this as dynamic for now to unblock but ideally the arrow built dependencies should be used.
# NOTE: No longer need to install snappy since we are using arrow's bundled version of snappy. 
# If we do end up needing it, we should also add it to the ubuntu setup scripts for consistency.
# run_step "[ airtree-setup ] Install snappy via vcpkg" "$VCPKG_DIR/vcpkg.exe" install snappy:x64-windows

export CMAKE_TOOLCHAIN_FILE="$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake"
echo "export CMAKE_TOOLCHAIN_FILE=\"$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake\"" >> /tmp/env.sh

log "INFO" "Windows setup completed successfully! MSVC configured at $(which cl.exe)"
