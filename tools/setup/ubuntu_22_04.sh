#! /usr/bin/env bash

set -eou pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT_DIR="${ROOT_DIR%/tools*}"
. "$ROOT_DIR/tools/utils/import.sh"
import utils/common_func.sh

OS="$(uname)"
ARCH="$(uname -m)"

# Fetch distro
if [[ "$OS" == "Linux" ]]; then
  if [[ -f /etc/os-release ]]; then
    . /etc/os-release
    DISTRO=$ID
    VERSION=$(lsb_release -rs)
  else
    error_exit "Unable to determine Linux distribution."
  fi
fi

# This script is specifically for Ubuntu 22.04 on x86_64 architecture
if [[ "$OS" != "Linux" || "$ARCH" != "x86_64" || "$DISTRO" != "ubuntu" || "$VERSION" != "22.04" ]]; then
  error_exit "This script is intended for Ubuntu 22.04 on x86_64 architecture."
fi

log "INFO" "Installing dependencies on $OS $DISTRO ($VERSION) $ARCH ..."

if [ "$(which sudo)" != "" ]; then
    SUDO=sudo
else
    # If sudo doesn't exist, assume we don't need it.
    # The AWS build environment doesn't have it.
    SUDO=
fi

# Helper function to install a package and check for errors
install_package() {
  local package=$1
  # NOTE: use apt-get in scripts since it has a more stable interface
  $SUDO apt-get install -y "$package"
  if [[ $? -ne 0 ]]; then
    echo "Failed to install $package."
    exit 1
  fi
}

run_step "[ airtree-setup ] Run apt-get update" $SUDO apt-get update
run_step "[ airtree-setup ] Install gcc" install_package gcc # should version lock this
run_step "[ airtree-setup ] Install g++" install_package g++ # should version lock this
run_step "[ airtree-setup ] Install cppcheck" install_package cppcheck
run_step "[ airtree-setup ] Install clang-tools-11" install_package clang-tools-11
run_step "[ airtree-setup ] Install build-essential" install_package build-essential
run_step "[ airtree-setup ] Install wget" install_package wget
run_step "[ airtree-setup ] Install curl" install_package curl
run_step "[ airtree-setup ] Install unzip" install_package unzip

# installing cmake
# minimum CMake version required
CMAKE_REQUIRED="3.27.9"
CMAKE_VERSION="3.27"
CMAKE_VERSION_FULL="3.27.9"
CMAKE_DIR=/opt/cmake-$CMAKE_VERSION
if ! command -v cmake >/dev/null 2>&1; then
    echo "Error: CMake is not installed or not in PATH" >&2
    CMAKE_INSTALLED="0.0.0"
else
  CMAKE_INSTALLED=$(cmake --version | head -n1 | awk '{print $3}')
fi
# comparing versions using `sort -V` - `sort -V`` understands version numbers
if [[ "$(printf '%s\n%s' "$CMAKE_REQUIRED" "$CMAKE_INSTALLED" | sort -V | head -n1)" != $CMAKE_REQUIRED ]]; then
  log "WARNING" "CMake version $CMAKE_INSTALLED is installed, but version $CMAKE_REQUIRED is required."
  log "INFO" "Installing the required version of CMake..."
  run_step "[ airtree-setup ] Remove existing CMake version" $SUDO apt-get remove -y cmake # remove any existing version

  mkdir -p /tmp/cmake_install
  pushd /tmp/cmake_install
  wget -q https://cmake.org/files/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION_FULL}-linux-x86_64.tar.gz
  tar -xzf cmake-${CMAKE_VERSION_FULL}-linux-x86_64.tar.gz
  $SUDO mv cmake-${CMAKE_VERSION_FULL}-linux-x86_64 $CMAKE_DIR
  popd

  export PATH=$CMAKE_DIR/bin:$PATH
  # Write to an env file to persist across phases
  echo "export PATH=$CMAKE_DIR/bin:\$PATH" >> /tmp/env.sh
else
  log "INFO" "CMake version $CMAKE_INSTALLED is already installed and meets the requirement."
fi

# install python3.12 by specifying the apt-get repository
run_step "[ airtree-setup ] Add deadsnakes PPA" $SUDO add-apt-repository ppa:deadsnakes/ppa -y > /dev/null
run_step "[ airtree-setup ] Update package list" $SUDO apt-get update
run_step "[ airtree-setup ] Install python3.12" install_package python3.12
run_step "[ airtree-setup ] Install python3.12-venv" install_package python3.12-venv
run_step "[ airtree-setup ] Install python3.12-dev" install_package python3.12-dev

# Docker is unused by the build/tests — disabled pending full removal once CI confirms.
# run_step "[ airtree-setup ] Install Docker" "$TOOLS_DIR/setup/install_docker.sh"

run_step "[ airtree-setup ] Install ccache" install_package ccache
run_step "[ airtree-setup ] Install ninja-build" install_package ninja-build
run_step "[ airtree-setup ] Install pkg-config" install_package pkg-config
run_step "[ airtree-setup ] Install git" install_package git
# install_package python3-dev # We don't need both - we should instead specify a default (which is 3.12 in our case)
# install_package python3.12-dev
# No longer needed since we are using arrow's bundled version of snappy
# run_step "[ airtree-setup ] Install libsnappy-dev" install_package libsnappy-dev
run_step "[ airtree-setup ] Install bison" install_package bison
run_step "[ airtree-setup ] Install flex" install_package flex

# Configure compiler
GCC_PATH=$(which gcc)
GPP_PATH=$(which g++)
if [ -z "$GCC_PATH" ] || [ -z "$GPP_PATH" ]; then
  error_exit "gcc and g++ are required but not found. Please install them."
fi
export CC="$GCC_PATH"
export CXX="$GPP_PATH"
log "INFO" "Using GCC at $CC and G++ at $CXX"

# Install AWS CLI v2
if command -v aws >/dev/null 2>&1; then
  log "INFO" "AWS CLI already installed: $(aws --version)"
else
  log "INFO" "Installing AWS CLI v2..."

  TMP_DIR=$(mktemp -d)
  pushd "$TMP_DIR" >/dev/null

  # Download AWS CLI v2
  run_step "[ airtree-setup ] Download AWS CLI v2" curl "https://awscli.amazonaws.com/awscli-exe-linux-x86_64.zip" -o "awscliv2.zip"
  run_step "[ airtree-setup ] Unzip AWS CLI v2" unzip awscliv2.zip

  # Install to /usr/local/aws-cli and symlink to /usr/local/bin/aws
  run_step "[ airtree-setup ] Install AWS CLI v2" $SUDO ./aws/install --update

  popd >/dev/null
  rm -rf "$TMP_DIR"

  log "INFO" "Installed AWS CLI: $(aws --version)"
fi
