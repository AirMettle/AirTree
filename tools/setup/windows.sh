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
  run_step --retries 4 --retry-delay 30 "[ airtree-setup ] Install $package" choco install "$package" $version_flag -y --no-progress
}

# Install via choco only when the tool is missing. GitHub-hosted runners
# preinstall most of these, and the community choco feed rate-limits CI IPs
# aggressively (503s), so every skipped install matters.
ensure_package() {
  local cmd=$1 package=$2
  if command -v "$cmd" >/dev/null 2>&1; then
    log "INFO" "$package already available ($(command -v "$cmd")). Skipping choco install."
  else
    install_package "$package"
  fi
}

ensure_package wget wget
ensure_package curl curl
ensure_package unzip unzip
# Can't install git via Chocolatey using git bash on windows since we are already running git bash :). I'm going to
# skip this and add it as a hard requirement for windows users to have git installed and in their PATH.
# install_package "git"
ensure_package cppcheck cppcheck
ensure_package ninja ninja
ensure_package sccache sccache
ensure_package win_flex winflexbison3
ensure_package pkg-config pkgconfiglite
ensure_package nasm nasm

# OpenSSL's VC-WIN64A build needs a native (non-MSYS) perl; git-bash's own
# perl reports $^O=msys and doesn't count, but a Strawberry install dir does.
if [ -x "/c/Strawberry/perl/bin/perl.exe" ] \
    || { command -v perl >/dev/null 2>&1 && [ "$(perl -e 'print $^O')" = "MSWin32" ]; }; then
  log "INFO" "Native perl already available. Skipping choco install."
else
  install_package "strawberryperl"
fi

# Any cmake 3.22+ works (project minimum); 4.x is rejected since some
# third-party deps still declare pre-3.5 minimums that 4.x refuses to build.
CMAKE_REQUIRED="3.27.9"
if command -v cmake >/dev/null 2>&1 && cmake --version | head -1 | grep -qE ' 3\.(2[2-9]|[3-9][0-9])'; then
  log "INFO" "$(cmake --version | head -1) already available. Skipping pinned install."
else
  install_package "cmake" "--version $CMAKE_REQUIRED --force"
  export PATH="/c/Program Files/CMake/bin:$PATH"
fi

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


log "INFO" "Windows setup completed successfully! MSVC configured at $(which cl.exe)"
