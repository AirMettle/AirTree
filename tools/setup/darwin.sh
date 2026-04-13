#! /usr/bin/env bash

set -eou pipefail
OS="$(uname)"
ARCH="$(uname -m)"

if [[ "$OS" != "Darwin" ]]; then
  echo "This script is intended for macOS (Darwin) only."
  exit 1
fi

# Ensure Homebrew is installed
if ! command -v brew >/dev/null 2>&1; then
  echo "Homebrew is required but not installed."
  echo "Install it from https://brew.sh"
  exit 1
fi

# Helper function to install a package and check for errors
install_package() {
  local package=$1
  brew install "$package"
  if [[ $? -ne 0 ]]; then
    echo "Failed to install $package."
    exit 1
  fi
}

echo "Installing dependencies on $OS $ARCH ..."

# Bash 4+ is required for associative arrays (declare -A) used by import.sh.
# macOS ships bash 3.2 — install a modern version via Homebrew.
if [[ "${BASH_VERSINFO[0]}" -lt 4 ]]; then
  echo "Bash ${BASH_VERSION} detected — version 4.0+ is required."
  echo "Installing modern bash via Homebrew..."
  install_package bash
  echo ""
  echo "Homebrew bash installed. Ensure it is first in your PATH:"
  echo "  export PATH=\"/opt/homebrew/bin:\$PATH\""
  echo "Then re-run this script."
  exit 1
fi

install_package pkg-config
install_package gcc@14
install_package git
install_package cmake
install_package ninja
# This is probably no longer needed - since we are building from source.
# needs to be validated on mac
# install_package boost

# Configure compiler
GCC_PATH=$(which gcc-14 || true)
GPP_PATH=$(which g++-14 || true)
if [ -z "$GCC_PATH" ] || [ -z "$GPP_PATH" ]; then
  echo "gcc-14 and g++-14 are required but not found. Please install them using:"
  echo "brew install gcc@14"
  exit 1
fi
export CC="$GCC_PATH"
export CXX="$GPP_PATH"
echo "Using GCC at $CC and G++ at $CXX"

# xcode-select --install || echo "Command line tools are already installed."
