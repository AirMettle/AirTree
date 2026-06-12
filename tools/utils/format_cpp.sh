#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT_DIR="${ROOT_DIR%/tools*}"
. "$ROOT_DIR/tools/utils/import.sh"
import utils/common_func.sh

#Parse flags

VERBOSE=false
if [[ "${1:-}" == "--verbose" ]]; then
  VERBOSE=true
fi

#Find and cd to the repo root

if ! REPO_ROOT=$(git rev-parse --show-toplevel 2>/dev/null); then
  echo "ERROR: not inside a Git repository." >&2
  exit 1
fi
cd "$REPO_ROOT"

#Verify clang‑format is installed if not do it.
if ! command -v clang-format &>/dev/null; then
  echo "clang-format not found. Attempting to install…"
  if command -v apt-get &>/dev/null; then
    SUDO=$(determine_sudo)
    $SUDO apt-get update && $SUDO apt-get install -y clang-format
  elif command -v brew &>/dev/null; then
    brew install clang-format
  else
    echo "ERROR: No supported package manager found. Please install clang-format manually." >&2
    exit 1
  fi
fi


# Gather all C/C++ files inside the repo

mapfile -t files < <(
  find . \
    -type f \
    \( -iname '*.cpp' -o -iname '*.cc' -o -iname '*.cxx' \
       -o -iname '*.hpp' -o -iname '*.h'  -o -iname '*.hxx' \) \
    -not -path './build/*' \
    -not -path './.git/*' \
    -not -path './venv/*'
)
echo "Found ${#files[@]} C/C++ files under $REPO_ROOT"

#Format them (verbose or batch)
if [[ "$VERBOSE" == true ]]; then
  for file in "${files[@]}"; do
    echo "Formatting: $file"
    clang-format -i --style=file "$file"
  done
else
  printf '%s\0' "${files[@]}" | xargs -0 clang-format -i --style=file
fi
echo "FORMATTING CHECK PASSED"