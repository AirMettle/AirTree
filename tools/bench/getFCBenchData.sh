#!/bin/bash

#this script downloads dataset files from gdown https://drive.google.com/drive/folders/1jdnzwvT1hya8XYdEJ7QuqUw3ALbQozc7
#More infomormation about data at: https://github.com/hpdps-group/FCBench#datasets

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
nyc_taxi=1ODXvl_gsohxv4z29aNfL0gk458fiZYYO
gas_price=1n4ihLBaIbQji2iMjlAzTDL1_ryhG6E5z
hurricane=1h48gO2JNCNWMVaEPsIHPBkNllgtHDzcf
solar_wind=1sVAEV0wLdfrrXFm6uQKvYU0412vKlaQa

#Functions:
ensure_gdown() {
    if command -v gdown >/dev/null 2>&1; then
        return 0
    fi

    echo "Installing gdown..."

    local installed=false

    if command -v uv >/dev/null 2>&1; then
        uv tool install gdown && installed=true
    elif command -v pipx >/dev/null 2>&1; then
        pipx install gdown && installed=true
    elif command -v python3 >/dev/null 2>&1; then
        python3 -m pip install --user gdown && installed=true
    fi

    if $installed && command -v gdown >/dev/null 2>&1; then
        echo "gdown installed successfully"
        return 0
    else
        echo "Failed to install gdown"
        return 1
    fi
}

#name path to file destination
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT_DIR="${ROOT_DIR%/tools*}"

. "$ROOT_DIR/tools/utils/import.sh"
import utils/build_utils.sh

#destination directory is the build directory ({build_dir}/bench_data)
_CMAKE_BUILD_DIR="$CMAKE_BUILD_DIR"

#check if gdown is installed or install (reccomended tool for downloading datasets from google drive)
ensure_gdown

#destination dir:
mkdir -p "$_CMAKE_BUILD_DIR/bench_data"

#downloads:
echo "downloading msg-bt dataset..."
if gdown "https://drive.google.com/uc?id=$msg_bt" \
      -O "$_CMAKE_BUILD_DIR/bench_data/msg_bt.bin" ; then
    echo "download successful"
else
    echo "download failed."

fi

echo "Downloading astro-mhd dataset..."
if gdown "https://drive.google.com/uc?id=$astro_mhd" \
         -O "$_CMAKE_BUILD_DIR/bench_data/astro_mhd.bin"; then
    echo "download successful"
else
    echo "download failed"
fi

echo "Downloading astro-pt dataset..."
if gdown "https://drive.google.com/uc?id=$astro_pt" \
         -O "$_CMAKE_BUILD_DIR/bench_data/astro_pt.bin"; then
    echo "download successful"
else
    echo "download failed"
fi

echo "Downloading num-control dataset..."
if gdown "https://drive.google.com/uc?id=$num_control" \
         -O "$_CMAKE_BUILD_DIR/bench_data/num_control.bin"; then
    echo "download successful"
else
    echo "download failed"
fi

echo "Downloading num-brain dataset..."
if gdown "https://drive.google.com/uc?id=$num_brain" \
         -O "$_CMAKE_BUILD_DIR/bench_data/num_brain.bin"; then
    echo "download successful"
else
    echo "download failed"
fi

echo "Downloading phone-gyro dataset..."
if gdown "https://drive.google.com/uc?id=$phone_gyro" \
         -O "$_CMAKE_BUILD_DIR/bench_data/phone_gyro.bin"; then
    echo "download successful"
else
    echo "download failed"
fi

echo "Downloading wesad-chest dataset..."
if gdown "https://drive.google.com/uc?id=$wesad_chest" \
         -O "$_CMAKE_BUILD_DIR/bench_data/wesad_chest.bin"; then
    echo "download successful"
else
    echo "download failed"
fi

echo "Downloading jane-street dataset..."
if gdown "https://drive.google.com/uc?id=$jane_street" \
         -O "$_CMAKE_BUILD_DIR/bench_data/jane_street.bin"; then
    echo "download successful"
else
    echo "download failed"
fi

echo "Downloading nyc-taxi dataset..."
if gdown "https://drive.google.com/uc?id=$nyc_taxi" \
         -O "$_CMAKE_BUILD_DIR/bench_data/nyc_taxi.bin"; then
    echo "download successful"
else
    echo "download failed"
fi

echo "Downloading gas-price dataset..."
if gdown "https://drive.google.com/uc?id=$gas_price" \
         -O "$_CMAKE_BUILD_DIR/bench_data/gas_price.bin"; then
    echo "download successful"
else
    echo "download failed"
fi

echo "Downloading hurricane dataset..."
if gdown "https://drive.google.com/uc?id=$hurricane" \
         -O "$_CMAKE_BUILD_DIR/bench_data/hurricane.bin"; then
    echo "download successful"
else
    echo "download failed"
fi

echo "Downloading solar-wind dataset..."
if gdown "https://drive.google.com/uc?id=$solar_wind" \
         -O "$_CMAKE_BUILD_DIR/bench_data/solar_wind.bin"; then
    echo "download successful"
else
    echo "download failed"
fi

echo "All downloads completed."
