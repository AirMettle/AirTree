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

result="$tmp/maxcount.txt"
"$cli" query max_count -i "$out" -o "$result"
if [[ ! -s "$result" ]]; then
  echo "FAIL: query results file $result is empty" >&2
  exit 1
fi
if ! grep -q "^Bin:" "$result"; then
  echo "FAIL: query results file lacks expected 'Bin:' line" >&2
  cat "$result" >&2
  exit 1
fi

percentile="$tmp/percentile.txt"
"$cli" query percentile -i "$out" -o "$percentile" -p 50.0
if ! grep -q "^Percentile:" "$percentile"; then
  echo "FAIL: percentile results file lacks expected 'Percentile:' line" >&2
  cat "$percentile" >&2
  exit 1
fi

echo "PASS: histogram size $(stat -c %s "$out") bytes; query/percentile results captured"
