#!/usr/bin/env bash
# Regression test for parquet_handler writing the histogram buffer.
# See airtree-cli/src/cli/AirTree.cpp::parquet_handler.
set -euo pipefail

cli="$1"
parquet="$2"

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

out="$tmp/hist.bin"
"$cli" generate parquet -i "$parquet" -o "$out" -s 1DxP -c value

if [[ ! -s "$out" ]]; then
  echo "FAIL: histogram output $out is empty" >&2
  exit 1
fi

echo "PASS: histogram size $(stat -c %s "$out") bytes"
