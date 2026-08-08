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

echo "Working directory (query consolidate): $DIR"

# Only schemas that produce query CSVs in query_bench_planv2
schemas=(1DxT 1DxF 1DxP 2DxP 3DxP 4DxP)

all_temp="$DIR/consolidated_query_all_data.csv.tmp"
> "$all_temp"
all_header=""
all_found=false

rewrite_query_csv_rows() {
    local file="$1"
    local dataset_name="$2"
    local schema="$3"
    awk -v ds="$dataset_name" -v sch="$schema" -v src="$file" '
        BEGIN {
            FS = ","
            OFS = ","
            in_data = 0
            name_col = 0
            query_id_col = 0
            nfields = 0
        }
        function strip_quotes(s) {
            gsub(/^"/, "", s)
            gsub(/"$/, "", s)
            return s
        }
        function build_header(    i, h, parts, out, seen_query_id, name) {
            nfields = split($0, parts, ",")
            name_col = 0
            query_id_col = 0
            for (i = 1; i <= nfields; i++) {
                h = strip_quotes(parts[i])
                colname[i] = h
                if (h == "name") name_col = i
                if (h == "query_id") query_id_col = i
            }
            if (name_col == 0) {
                print "Error: no name column in header of " src > "/dev/stderr"
                exit 1
            }
            if (query_id_col == 0) {
                print "Error: no query_id counter column in header of " src > "/dev/stderr"
                exit 1
            }
            out = "Data_set,Schema,query_id"
            seen_query_id = 0
            for (i = 1; i <= nfields; i++) {
                if (i == name_col) continue
                if (i == query_id_col) continue
                name = colname[i]
                if (name == "query_id") continue
                out = out "," name
            }
            return out
        }
        function emit_row(    i, out, qid, first) {
            if (NF < nfields) {
                # GB may omit trailing empty fields; pad
                while (NF < nfields) {
                    $(NF + 1) = ""
                }
            }
            qid = strip_quotes($query_id_col)
            if (qid == "" || qid == "query_id") {
                print "Error: missing query_id on row in " src " (name=" $name_col ")" > "/dev/stderr"
                exit 1
            }
            out = ds OFS sch OFS qid
            for (i = 1; i <= nfields; i++) {
                if (i == name_col) continue
                if (i == query_id_col) continue
                if (colname[i] == "query_id") continue
                out = out OFS $i
            }
            print out
        }
        /name,iterations,real_time,cpu_time/ {
            if (!in_data) {
                header_out = build_header()
                # Only used by caller when capturing header; rows printed below
                header_printed = 0
            }
            in_data = 1
            next
        }
        in_data && $1 ~ /^"?(AirTreeQuery)/ {
            emit_row()
        }
    ' "$file"
}

# Extract rewritten header from first file (same awk logic, header only)
extract_query_header() {
    local file="$1"
    awk '
        BEGIN { FS = "," }
        function strip_quotes(s) {
            gsub(/^"/, "", s)
            gsub(/"$/, "", s)
            return s
        }
        /name,iterations,real_time,cpu_time/ {
            nfields = split($0, parts, ",")
            name_col = 0
            query_id_col = 0
            for (i = 1; i <= nfields; i++) {
                h = strip_quotes(parts[i])
                colname[i] = h
                if (h == "name") name_col = i
                if (h == "query_id") query_id_col = i
            }
            if (name_col == 0 || query_id_col == 0) {
                print "Error: header missing name or query_id in " FILENAME > "/dev/stderr"
                exit 1
            }
            out = "Data_set,Schema,query_id"
            for (i = 1; i <= nfields; i++) {
                if (i == name_col) continue
                if (i == query_id_col) continue
                if (colname[i] == "query_id") continue
                out = out "," colname[i]
            }
            print out
            exit
        }
    ' "$file"
}

for schema in "${schemas[@]}"; do
    echo "Processing query schema: $schema"

    consolidated="$DIR/consolidated_query_${schema}_data.csv"
    tempfile="${consolidated}.tmp"

    > "$tempfile"

    found=false
    header_line=""

    # Enable nullglob-like behavior: skip if no matches
    shopt -s nullglob
    files=("$DIR"/query_*_"${schema}".csv)
    shopt -u nullglob

    for file in "${files[@]}"; do
        if [ ! -f "$file" ]; then
            continue
        fi

        found=true
        all_found=true

        filename=$(basename "$file")
        # query_yellow_tripdata_1DxF.csv → yellow_tripdata
        # query_jane_street_1DxT.csv → jane_street
        dataset_name=$(echo "$filename" | sed -e 's/^query_//' -e "s/_${schema}\\.csv$//")

       
        if [ -z "$header_line" ]; then
            header_line=$(extract_query_header "$file")
        fi

        if [ -z "$all_header" ]; then
            all_header="$header_line"
        fi

        rewrite_query_csv_rows "$file" "$dataset_name" "$schema" >> "$tempfile"
        rewrite_query_csv_rows "$file" "$dataset_name" "$schema" >> "$all_temp"
        
    done

    if [ "$found" = true ]; then
        
        # header_line already includes Data_set,Schema,query_id,...
        echo "${header_line}" > "$consolidated"
        
        if [ -s "$tempfile" ]; then
            cat "$tempfile" >> "$consolidated"
        fi
        row_count=$(($(wc -l < "$consolidated") - 1))
        echo "Created $consolidated with $row_count data rows."
    else
        echo "No query files found for schema $schema."
    fi

    rm -f "$tempfile"
done

if [ "$all_found" = true ] && [ -n "$all_header" ]; then
    all_out="$DIR/consolidated_query_all_data.csv"
    
    echo "${all_header}" > "$all_out"
    
    cat "$all_temp" >> "$all_out"
    row_count=$(($(wc -l < "$all_out") - 1))
    echo "Created $all_out with $row_count data rows."
fi
rm -f "$all_temp"

echo "Query consolidation of files complete."