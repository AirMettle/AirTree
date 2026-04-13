#!/usr/bin/env bash

# shellcheck disable=SC1091

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT_DIR="${ROOT_DIR%/tools*}"
. "$ROOT_DIR/tools/utils/import.sh"
import utils/common_func.sh
import utils/build_utils.sh

VERSION_FILE="$PROJECT_ROOT/version.txt"
README_FILE="$PROJECT_ROOT/README.md"

# Read version from version.txt
if [[ ! -f "$VERSION_FILE" ]]; then
    log_error "version.txt not found at $VERSION_FILE"
    exit 1
fi

VERSION=$(cat "$VERSION_FILE" | tr -d '[:space:]')
log_info "Current version: $VERSION"

# Check README.md exists
if [[ ! -f "$README_FILE" ]]; then
    log_error "README.md not found at $README_FILE"
    exit 1
fi

# URL encode the version for shields.io badge (replace - with --)
VERSION_ENCODED="${VERSION//-/--}"

# Create backup
cp "$README_FILE" "$README_FILE.bak"

# Update README.md badge using sed
# Pattern matches: version-X.Y.Z--SNAPSHOT-blue.svg or version-X.Y.Z-SNAPSHOT-blue.svg
run_step "Updating version badge in README.md" \
    sed -i.tmp -E "s#(https://img\.shields\.io/badge/version-)([0-9]+\.[0-9]+\.[0-9]+)(--|-)(SNAPSHOT)#\1${VERSION_ENCODED}#g" "$README_FILE"

# Remove temporary file created by sed
rm -f "$README_FILE.tmp"

# Check if anything changed
if diff -q "$README_FILE" "$README_FILE.bak" > /dev/null 2>&1; then
    log_info "No changes needed - version already up to date"
    rm "$README_FILE.bak"
else
    log_success "Updated README.md badge to version $VERSION"
    rm "$README_FILE.bak"
fi
