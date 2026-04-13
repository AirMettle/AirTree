#!/usr/bin/env bash

set -e

if [ "$(which sudo)" != "" ]; then
  SUDO_BIN=sudo
else
  # If sudo doesn't exist, assume we don't need it.
  # The AWS build environment doesn't have it.
  SUDO_BIN=
fi

printf "=== Docker Install ===\n"

# Compare two dotted version strings (returns 0 if $1 >= $2)
_version_ge() {
  local IFS=.
  local i v1=($1) v2=($2)
  for ((i=0; i<${#v2[@]}; i++)); do
    if (( ${v1[i]:-0} < ${v2[i]:-0} )); then return 1; fi
    if (( ${v1[i]:-0} > ${v2[i]:-0} )); then return 0; fi
  done
  return 0
}

# Check Docker version
PACKAGE_NAME="docker-ce"
REQUIRED_MIN_VERSION="24.0"

if DPKG_VERSION_STR="$(dpkg-query --show --showformat='${Version}' ${PACKAGE_NAME} 2>/dev/null)"; then
  if [ ! "${DPKG_VERSION_STR}" == "" ]; then
    VERSION=${DPKG_VERSION_STR#*:}
    VERSION=${VERSION%%-*}
    if _version_ge "${VERSION}" "${REQUIRED_MIN_VERSION}"; then
      printf "%s installed OK (version: '%s')\n" $PACKAGE_NAME "${DPKG_VERSION_STR}"
      exit 0
    else
      printf "%s installed version not OK (required min version: %s, installed version: %s)\n" $PACKAGE_NAME ${REQUIRED_MIN_VERSION} "${DPKG_VERSION_STR}"
    fi
  else
    printf "%s not installed\n" $PACKAGE_NAME
  fi
else
  printf "%s package not found\n" $PACKAGE_NAME
fi

# Install
printf "Installing %s...\n" $PACKAGE_NAME

# Remove old repository (if it is there)
$SUDO_BIN add-apt-repository --remove --yes "deb https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"

# Add keys
$SUDO_BIN apt-get update
$SUDO_BIN apt-get -y install ca-certificates curl gnupg
$SUDO_BIN install -m 0755 -d /etc/apt/keyrings
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | $SUDO_BIN gpg --yes --dearmor -o /etc/apt/keyrings/docker.gpg
$SUDO_BIN chmod a+r /etc/apt/keyrings/docker.gpg

# Add repo
echo \
  "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu \
  $(. /etc/os-release && echo "$VERSION_CODENAME") stable" | \
  $SUDO_BIN tee /etc/apt/sources.list.d/docker.list > /dev/null
$SUDO_BIN apt-get update

# Install
$SUDO_BIN apt-get -y install \
  docker-ce \
  docker-ce-cli \
  containerd.io \
  docker-buildx-plugin \
  docker-compose-plugin
