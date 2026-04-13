#! /usr/bin/env bash

set -euo pipefail

# Script with helpers to fetch large files from remote locations.
# Usage: source this script and call the functions below.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT_DIR="${ROOT_DIR%/tools*}"
. "$ROOT_DIR/tools/utils/import.sh"
import utils/common_func.sh


# Function to fetch a file from S3 using AWS CLI
# Arguments:
#   1. S3 URI (e.g., s3://bucket-name/path/to/file)
#   2. Local destination path
fetch_from_s3() {
    local s3_uri="$1"
    local dest_path="$2"

    aws s3 cp "$s3_uri" "$dest_path"
    log "INFO" "Fetched $s3_uri to $dest_path"
}
