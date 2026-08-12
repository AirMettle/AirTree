#! /usr/bin/env bash

set -eou pipefail

# Guard before import.sh: it needs bash 4+ (declare -A), macOS ships bash 3.2.
if [[ "${BASH_VERSINFO[0]}" -lt 4 ]]; then
  echo "Bash ${BASH_VERSION} detected — version 4.0+ is required." >&2
  echo "Install with 'brew install bash', ensure it is first in PATH, then re-run." >&2
  exit 1
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT_DIR="${ROOT_DIR%/tools*}"
. "$ROOT_DIR/tools/utils/import.sh"
import utils/common_func.sh

OS="$(uname | awk '{ print tolower($0) }')"
ARCH="$(uname -m | awk '{ print tolower($0) }')"

if [[ "$OS" != "darwin" ]]; then
  error_exit "This script is intended for macOS (Darwin) only."
fi

log "INFO" "Installing dependencies on macOS ($OS) $ARCH ..."

if ! command -v brew >/dev/null 2>&1; then
  error_exit "Homebrew is required but not installed. Install it from https://brew.sh"
fi

install_package() {
  local package=$1
  run_step --retries 2 --retry-delay 15 "[ airtree-setup ] Install $package" brew install "$package"
}

# Install via brew only when the tool is missing. GitHub-hosted runners
# preinstall most of these, so every skipped install matters.
ensure_package() {
  local cmd=$1 package=$2
  if command -v "$cmd" >/dev/null 2>&1; then
    log "INFO" "$package already available ($(command -v "$cmd")). Skipping brew install."
  else
    install_package "$package"
  fi
}

ensure_package git git
ensure_package ninja ninja
ensure_package pkg-config pkgconf
ensure_package cppcheck cppcheck
ensure_package aws awscli

# Version-locked compiler: Homebrew LLVM 20 (see cmake/Toolchain_clang_20.cmake).
LLVM20_PREFIX="$(brew --prefix llvm@20)"
if [[ -x "$LLVM20_PREFIX/bin/clang" ]]; then
  log "INFO" "LLVM 20 already available at $LLVM20_PREFIX. Skipping brew install."
else
  install_package llvm@20
fi

# python@3.12 may be keg-only, so also probe its keg path (venv.sh does the same).
if command -v python3.12 >/dev/null 2>&1 || [[ -x "$(brew --prefix python@3.12)/bin/python3.12" ]]; then
  log "INFO" "Python 3.12 already available. Skipping brew install."
else
  install_package python@3.12
fi

# Any cmake 3.22+ works (project minimum); 4.x is rejected since some
# third-party deps still declare pre-3.5 minimums that 4.x refuses to build.
CMAKE_VERSION="3.27"
CMAKE_VERSION_FULL="3.27.9"
CMAKE_DIR="/opt/cmake-$CMAKE_VERSION"
if command -v cmake >/dev/null 2>&1 && cmake --version | head -1 | grep -qE ' 3\.(2[2-9]|[3-9][0-9])'; then
  log "INFO" "$(cmake --version | head -1) already available. Skipping pinned install."
else
  SUDO=$(determine_sudo)
  if [[ ! -x "$CMAKE_DIR/CMake.app/Contents/bin/cmake" ]]; then
    TMP_DIR=$(mktemp -d)
    run_step "[ airtree-setup ] Download CMake $CMAKE_VERSION_FULL" \
      curl -fsSL "https://cmake.org/files/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION_FULL}-macos-universal.tar.gz" -o "$TMP_DIR/cmake.tar.gz"
    run_step "[ airtree-setup ] Extract CMake" tar -xzf "$TMP_DIR/cmake.tar.gz" -C "$TMP_DIR"
    run_step "[ airtree-setup ] Install CMake to $CMAKE_DIR" $SUDO mv "$TMP_DIR/cmake-${CMAKE_VERSION_FULL}-macos-universal" "$CMAKE_DIR"
    rm -rf "$TMP_DIR"
  fi
  export PATH="$CMAKE_DIR/CMake.app/Contents/bin:$PATH"
  # Write to an env file to persist across phases
  echo "export PATH=$CMAKE_DIR/CMake.app/Contents/bin:\$PATH" >> /tmp/env.sh
fi

if [[ ! -x "$LLVM20_PREFIX/bin/clang" ]]; then
  error_exit "LLVM 20 install incomplete: $LLVM20_PREFIX/bin/clang not found."
fi
log "INFO" "macOS setup completed successfully! clang configured at $LLVM20_PREFIX/bin/clang ($("$LLVM20_PREFIX/bin/clang" --version | head -1))"
