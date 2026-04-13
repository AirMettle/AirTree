#! /usr/bin/env bash

set -eou pipefail

OS=$(echo $(uname) | awk '{ print tolower($0) }')
ARCH=$(echo $(uname -m) | awk '{ print tolower($0) }')

# Fetch distro and version from /etc/os-release
if [[ "$OS" == "linux" ]]; then
  if [[ -f /etc/os-release ]]; then
    . /etc/os-release
    DISTRO=$(echo ${ID} | awk '{ print tolower($0) }')
    VERSION_ID=$(echo ${VERSION_ID} | awk '{ print tolower($0) }')
  else
    echo "Unable to determine Linux distribution."
    exit 1
  fi
else
  echo "This script is intended for Linux systems only."
  exit 1
fi

# Accept CentOS 7, 8, or 9 on x86_64
if [[ "$ARCH" != "x86_64" ]]; then
  echo "This script is intended for x86_64 architecture only."
  exit 1
fi

if [[ "$DISTRO" != "centos" ]]; then
  echo "This script is intended for CentOS only."
  exit 1
fi

if [[ "$VERSION_ID" != "7" && "$VERSION_ID" != "8" && "$VERSION_ID" != "9" ]]; then
  echo "This script is intended for CentOS 7, 8, or 9. Detected version: $VERSION_ID"
  exit 1
fi

if ! command -v yum >/dev/null 2>&1; then
  echo "yum package manager not found. This script requires yum."
  exit 1
fi

echo "Installing dependencies on $OS $DISTRO ($VERSION_ID) $ARCH ..."

# Helper function to install a package and check for errors
install_package() {
  local package=$1
  sudo yum install -y "$package"
  if [[ $? -ne 0 ]]; then
    echo "Failed to install $package."
    exit 1
  fi
}


sudo yum update -y
install_package gcc # FIX ME: version lock this
install_package g++ # FIX ME: version lock this
install_package epel-release
install_package cmake
install_package git

# The following packages were installed previously. They
# are probably no longer needed since we are building these
# from source in cmake/third-party
# install_package boost-devel
# install_package bzip2-devel
# install_package openssl-devel
# install_package asio-devel

# Configure compiler (try gcc-9/g++-9, fallback to system gcc/g++)
GCC_PATH=$(command -v gcc || true)
if [ -z "${GCC_PATH:-}" ]; then
  echo "gcc compiler not found. Installing it ..."
  install_package gcc
  GCC_PATH=$(command -v gcc)
fi

GPP_PATH=$(command -v g++ || true)
if [ -z "${GPP_PATH:-}" ]; then
  echo "g++ compiler not found. Installing it ..."
  install_package g++
  GPP_PATH=$(command -v g++)
fi

export CC="$GCC_PATH"
export CXX="$GPP_PATH"
echo "Using GCC at $CC and G++ at $CXX"
