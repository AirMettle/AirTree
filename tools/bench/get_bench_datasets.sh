#!/bin/bash

#this script downloads dataset files from:
# https://drive.google.com/drive/folders/1jdnzwvT1hya8XYdEJ7QuqUw3ALbQozc7
#  -More infomormation about data at: https://github.com/hpdps-group/FCBench#datasets
# https://www.nyc.gov/site/tlc/about/tlc-trip-record-data.page
# https://www.stlouisfed.org/research/economists/mccracken/fred-databases

# Dataset original formats:
# -FCBench: binary
# -yellow_tripdata: parquet
# -FRED: CSV converted to parquet via csv_to_parquet.py

set -euo pipefail

#Dataset and fileID from Google drive
msg_bt=15S7iTr_Yoo6oVv5TOemah0wP1K7VX5R1
astro_mhd=1gp2pUEtr8FP3g7hbu4EhDYtTyg2eVoBr
astro_pt=1ZI6h-8OOW2h7DG9L4P9tIGrUo_KBMH2R  
num_control=13Lpx_S0W4K5hBMOvyW61PFUOv9BXGOMN
num_brain=1D2WEJonO3GWQwAQxSokO6Pn4kffalhCy
phone_gyro=18WPrgYKUKg1vOuKatDgAQu7_2iDQ6EnK
wesad_chest=1v1Mz4ka_kmwFF5Bcb7QXg8pAYZ43_Ptv
jane_street=19JQgBJaLeHBaCV6G-Tcpqye8BQVWlqFt


dataset_ids=( "$msg_bt" "$astro_mhd" "$astro_pt" "$num_control" "$num_brain" "$phone_gyro" "$wesad_chest" "$jane_street" ) 
dataset_names=( "msg_bt" "astro_mhd" "astro_pt" "num_control" "num_brain" "phone_gyro" "wesad_chest" "jane_street" )

ensure_gdown() {
    if command -v gdown >/dev/null 2>&1; then
        return 0
    fi

    echo "Installing gdown..."

    python -m pip install --upgrade pip
    python -m pip install gdown

    if command -v gdown >/dev/null 2>&1; then
        echo "gdown installed successfully"
        return 0
    else
        echo "Failed to install gdown"
        return 1
    fi
}

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT_DIR="${ROOT_DIR%/tools*}"
. "$ROOT_DIR/tools/utils/import.sh"
import utils/common_func.sh

if [ $# -ne 1 ]; then
    log_error "Usage: $0 <destination_directory>"
    exit 1
fi

VENV_DIR=".venv"

if [ ! -d "$VENV_DIR" ];then

    echo "creating virtual environment"

    #supress gui selection 
    sudo DEBIAN_FRONTEND=noninteractive apt-get update -qq
    sudo DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends python3.10-venv
    python3 -m venv "$VENV_DIR"

fi

source "$VENV_DIR/bin/activate"

BENCH_DATA_DIR="$1"
mkdir -p "$BENCH_DATA_DIR"

#check if gdown is installed or install
ensure_gdown

#downloads:
echo "Dowloading datasets"

echo "Downloading FRED-MD dataset..."
if curl -L -o "$BENCH_DATA_DIR/2025-08-FRED-MD.csv" \
    "https://www.stlouisfed.org/-/media/project/frbstl/stlouisfed/research/fred-md/monthly/2025-08-md.csv?sc_lang=en&hash=78201C5ACC5082CC2600C8137A16CFA1"; then
    echo "download successful."
else
    echo "download failed."
fi

echo "Downloading FRED-QD dataset..."
if curl -L -o "$BENCH_DATA_DIR/2025-08-FRED-QD.csv" \
    "https://www.stlouisfed.org/-/media/project/frbstl/stlouisfed/research/fred-md/quarterly/2026-04-qd.csv?"; then
    
    echo "download successful."
else
    echo "download failed."
fi

#download all .bin datasets from FCBench
for index in "${!dataset_names[@]}"; do
    id=${dataset_ids[$index]}
    name=${dataset_names[$index]}
    
    echo "Downloading $name..."
    if gdown "https://drive.google.com/uc?id=$id" \
        -O "$BENCH_DATA_DIR/${name}.bin"; then
        echo "download successful."
    else
        echo "download failed."
    fi
done

#download nyc yellow taxi trip data for 2025
for month in {01..12}; do
    url="https://d37ci6vzurychx.cloudfront.net/trip-data/yellow_tripdata_2025-${month}.parquet"
    echo "Downloading yellow_tripdata_2025-${month}.parquet ..."
    curl -L -o "$BENCH_DATA_DIR/yellow_tripdata_2025-${month}.parquet" "$url"
done

#format or combine datasets 
if python3 -c "import pandas" &> /dev/null; then
    python3 csv_to_parquet.py "$BENCH_DATA_DIR"
    python3 combine_taxi_datasets.py "$BENCH_DATA_DIR"
else
    sudo DEBIAN_FRONTEND=noninteractive apt-get update -y
    sudo DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        python3-pip python3-dev
    pip3 install --no-cache-dir pandas pyarrow
    python3 csv_to_parquet.py "$BENCH_DATA_DIR"
    python3 combine_taxi_datasets.py "$BENCH_DATA_DIR"

fi

echo "retrieved datasets"