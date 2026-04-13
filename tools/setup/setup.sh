#!/usr/bin/env bash
# shellcheck disable=SC1091
set -eou pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT_DIR="${ROOT_DIR%/tools*}"
. "$ROOT_DIR/tools/utils/import.sh"

OS=$(uname | awk '{ print tolower($0) }')
ARCH=$(uname -m | awk '{ print tolower($0) }')

DISTRO=""
VERSION_ID=""

# Check OS distribution if on Linux
if [[ "$OS" == "linux" ]]; then
    if [[ -f /etc/os-release ]]; then
        source /etc/os-release
        DISTRO=$(echo "${ID}" | awk '{ print tolower($0) }')
        VERSION_ID=$(echo "${VERSION_ID}" | awk '{ print tolower($0) }')
    else
        echo "Unable to determine Linux distribution."
        exit 1
    fi
fi

PRE_BUILD_SETUP=0

if [[ "$OS" == "darwin" ]]; then
    . "$TOOLS_DIR/setup/darwin.sh"
    if [ $? -ne 0 ]; then
        echo "Darwin setup failed"
        exit 1
    fi
    PRE_BUILD_SETUP=1
fi

echo "OS: $OS, ARCH: $ARCH, DISTRO: $DISTRO, VERSION: $VERSION_ID"

if [[ "$OS" == "linux" && "$ARCH" == "x86_64" && "$DISTRO" == "ubuntu" && "$VERSION_ID" == "22.04" ]]; then
    . "$TOOLS_DIR/setup/ubuntu_22_04.sh"
    if [ $? -ne 0 ]; then
        echo "ubuntu_22_04 x86_64 setup failed"
        exit 1
    fi
    PRE_BUILD_SETUP=1
fi

if [[ "$OS" == "linux" && "$ARCH" == "aarch64" && "$DISTRO" == "ubuntu" && "$VERSION_ID" == "22.04" ]]; then
    . "$TOOLS_DIR/setup/ubuntu_22_04_arm.sh"
    if [ $? -ne 0 ]; then
        echo "ubuntu_22_04 aarch64 setup failed"
        exit 1
    fi
    PRE_BUILD_SETUP=1
fi

if [[ "$OS" == "linux" && "$ARCH" == "x86_64" && "$DISTRO" == "centos" && "$VERSION_ID" == "9" ]]; then
    . "$TOOLS_DIR/setup/centos_09.sh"
    if [ $? -ne 0 ]; then
        echo "centos_09 setup failed"
        exit 1
    fi

    PRE_BUILD_SETUP=1
fi


if [[ "$PRE_BUILD_SETUP" -eq 0 ]]; then
    echo "No pre-build setup script executed. Platform not supported."
    exit 1
else
    echo "Pre-build setup completed successfully."
fi

# TODO : Will revisit this later
# if ! . "$TOOLS_DIR/setup/install-hooks.sh"; then
#     echo "Failed to install Git hooks"
#     exit 1
# fi


echo "Setup completed successfully."
