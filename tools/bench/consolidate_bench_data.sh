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

for schema in "${schemas[@]}"; do
    echo "Processing schema: $schema"

    consolidated="$DIR/consolidated_${schema}_data.csv"
    tempfile="${consolidated}.tmp"

    > "$tempfile"

    found=false
    header_line=""

    for file in "$DIR"/*_"${schema}"*.csv; do
        if [ ! -f "$file" ]; then
            continue
        fi

        filename=$(basename "$file")
        # Skip query metrics and already-consolidated files
        [[ "$filename" == query_* ]] && continue
        [[ "$filename" == consolidated_* ]] && continue
        
        found=true

        dataset_name=$(echo "$filename" | sed "s/_${schema}.*$//")

        # Capture header only from the first file
        if [ -z "$header_line" ]; then
            header_line=$(awk '
                /name,iterations,real_time,cpu_time/ {
                    print $0
                    exit
                }
            ' "$file")
        fi

        # Extract data rows only
        awk -v ds="$dataset_name" '
            BEGIN {
                FS = ","
                OFS = ","
                in_data = 0
            }
            /name,iterations,real_time,cpu_time/ {
                in_data = 1
                next
            }
            in_data && $1 ~ /^"?(AirTreeBench)/ {
                # Remove surrounding quotes from first field if present
                gsub(/^"|"$/,"", $1)
                print ds "," $0
            }
        ' "$file" >> "$tempfile"
    done

    if [ "$found" = true ]; then
        # Write header ONLY ONCE
        echo "Data_set,${header_line}" > "$consolidated"
        
        # Append all collected data rows
        if [ -s "$tempfile" ]; then
            cat "$tempfile" >> "$consolidated"
        fi

        row_count=$(($(wc -l < "$consolidated") - 1))
        echo "Created $consolidated with $row_count data rows."
    else
        echo "No files found with $schema schema."
    fi

    rm -f "$tempfile"
done

echo "Consolidation of files complete."