#!/usr/bin/env bash
set -euo pipefail

if [[ "${_BOOTSTRAP_SOURCED:-}" == "true" ]]; then
    return 0
fi
_BOOTSTRAP_SOURCED=true

# Compute TOOLS_DIR from import.sh's own location (tools/utils/import.sh -> tools/)
TOOLS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export TOOLS_DIR

# Pure-bash path resolver — avoids dependency on realpath (missing/different on macOS)
_resolve_path() {
    (cd "$(dirname "$1")" 2>/dev/null && printf '%s/%s' "$(pwd)" "$(basename "$1")") || echo "$1"
}

# Now safe to declare the associative array and function
declare -A _SOURCED_FILES

import() {
    local file_path="$1"

    # Resolve non-absolute paths relative to TOOLS_DIR first
    if [[ "$file_path" != /* ]]; then
        if [[ -f "$TOOLS_DIR/$file_path" ]]; then
            file_path="$TOOLS_DIR/$file_path"
        elif [[ ! -f "$file_path" ]]; then
            echo "Error: File not found: $1" >&2
            echo "  Searched: $TOOLS_DIR/$1" >&2
            echo "  Searched: $1 (relative to CWD: $PWD)" >&2
            return 1
        fi
    fi

    # Normalize path for dedup key
    local file_key
    file_key="$(_resolve_path "$file_path")"

    if [[ "${_SOURCED_FILES[$file_key]:-}" == "true" ]]; then
        return 0
    fi

    if [[ -f "$file_path" ]]; then
        . "$file_path"
        _SOURCED_FILES[$file_key]="true"
    else
        echo "Error: File not found: $file_path" >&2
        return 1
    fi
}
