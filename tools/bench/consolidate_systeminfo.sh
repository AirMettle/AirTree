#!/bin/bash

set -euo pipefail

if [ $# -ne 1 ]; then
    echo "Error: usage $0 <working_dir>"
    exit 1
fi

DIR="$1"

if [ ! -d "$DIR" ]; then
    echo "Error: '$1' is not a directory"
    exit 1
fi

echo "Working directory: $DIR"

schemas=("1DxT" "1DxF" "1DxP" "2DxF" "2DxP" "3DxF" "3DxP" "4DxF" "4DxP")

consolidated="$DIR/systeminfo.csv"
tempfile="${consolidated}.tmp"

> "$tempfile"

found=false

for schema in "${schemas[@]}"; do
    for file in "$DIR"/*_"${schema}"_*.csv; do
        if [ ! -f "$file" ]; then
            continue
        fi

        found=true

        filename=$(basename "$file")
        dataset_name=$(echo "$filename" | sed "s/_${schema}_.*$//")
        row_label="${dataset_name}_${schema}_benchmark"

        awk -v label="$row_label" '
            BEGIN {
                ran_on = ""
                l1_data = ""
                l1_instr = ""
                l2 = ""
                l3 = ""
                load_avg = ""
            }
            /^Run on / {
                # "Run on (16 X 2300 MHz CPU s)" -> "16 X 2300 MHz CPU s"
                line = $0
                sub(/^Run on[[:space:]]+/, "", line)
                gsub(/^\(|\)$/, "", line)
                ran_on = line
                next
            }
            /^[[:space:]]*L1 Data / {
                line = $0
                sub(/^[[:space:]]*L1 Data[[:space:]]+/, "", line)
                l1_data = line
                next
            }
            /^[[:space:]]*L1 Instruction / {
                line = $0
                sub(/^[[:space:]]*L1 Instruction[[:space:]]+/, "", line)
                l1_instr = line
                next
            }
            /^[[:space:]]*L2 Unified / {
                line = $0
                sub(/^[[:space:]]*L2 Unified[[:space:]]+/, "", line)
                l2 = line
                next
            }
            /^[[:space:]]*L3 Unified / {
                line = $0
                sub(/^[[:space:]]*L3 Unified[[:space:]]+/, "", line)
                l3 = line
                next
            }
            /^Load Average: / {
                line = $0
                sub(/^Load Average:[[:space:]]*/, "", line)
                load_avg = line
                next
            }
            /^name,iterations,real_time,cpu_time/ {
                exit
            }
            END {
                # Skip empty/incomplete logs with no system header
                if (ran_on == "" && load_avg == "") {
                    exit
                }
                # Quote fields that may contain commas (Load Average)
                printf "%s,\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\n", \
                    label, ran_on, l1_data, l1_instr, l2, l3, load_avg
            }
        ' "$file" >> "$tempfile"
    done
done

if [ "$found" = true ]; then
    echo ",Ran on,L1 Data,L1 Instruction,L2 Unified,L3 Unified,Load Average" > "$consolidated"
    cat "$tempfile" >> "$consolidated"
    row_count=$(($(wc -l < "$consolidated") - 1))
    echo "Created $consolidated with $row_count data rows."
else
    echo "No benchmark CSV files found."
fi

rm -f "$tempfile"

echo "System info consolidation complete."
