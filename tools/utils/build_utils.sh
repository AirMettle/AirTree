#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT_DIR="${ROOT_DIR%/tools*}"
. "$ROOT_DIR/tools/utils/import.sh"

import utils/common_func.sh
import utils/logger.sh
import utils/color_codes.sh

import build/settings.sh

PROJECT_ROOT=$(get_project_root)
export PROJECT_ROOT

# Single source of truth for CMake source and build directories.
# Every script that needs these should source build_utils.sh instead of computing them inline.
CMAKE_SOURCE_DIR="$PROJECT_ROOT"
CMAKE_BUILD_DIR="$CMAKE_SOURCE_DIR/cmake-build-${TOOLCHAIN}-${BUILD_TYPE}-${SANITIZER}"
export CMAKE_SOURCE_DIR CMAKE_BUILD_DIR

get_project_version() {

  local _SNAPSHOT_TIMESTAMP
  _SNAPSHOT_TIMESTAMP=$(date +%Y%m%d-%H%M%S)

  local _VERSION_TXT
  _VERSION_TXT=$(cat "${PROJECT_ROOT}"/version.txt)

  local _VERSION=${_VERSION_TXT/-SNAPSHOT/}

  printf "%s" "${_VERSION}"
}

get_parallel_processes() {
    local memory_per_process_gb="${1:-2}"  # Default 2GB per process
    local reserved_memory_gb="${2:-}"      # Auto-calculate if not specified
    local cpu_usage_percent="${3:-80}"     # Use max 80% of CPU cores

    log_debug "Calculating parallel processes with ${memory_per_process_gb}GB per process"
    log_debug "Target CPU usage: ${cpu_usage_percent}%"

    # Get total memory in GB
    local total_memory_gb=0
    if [[ -f /proc/meminfo ]]; then
        total_memory_gb=$(awk '/MemTotal/ {print int($2/1024/1024)}' /proc/meminfo)
    elif command -v sysctl >/dev/null 2>&1; then
        local total_memory_bytes
        total_memory_bytes=$(sysctl -n hw.memsize 2>/dev/null || echo "0")
        total_memory_gb=$((total_memory_bytes / 1024 / 1024 / 1024))
    else
        log_warn "Cannot detect system memory, using conservative default"
        total_memory_gb=8
    fi

    # Get number of CPU cores
    local total_cpu_cores=1
    if command -v nproc >/dev/null 2>&1; then
        total_cpu_cores=$(nproc)
    elif command -v sysctl >/dev/null 2>&1; then
        total_cpu_cores=$(sysctl -n hw.ncpu 2>/dev/null || echo "1")
    else
        log_warn "Cannot detect CPU cores, defaulting to 1"
    fi

    # Calculate usable CPU cores (reserve some for system)
    local usable_cpu_cores=$((total_cpu_cores * cpu_usage_percent / 100))
    if [[ $usable_cpu_cores -lt 1 ]]; then
        usable_cpu_cores=1
    fi

    # Auto-calculate reserved memory if not specified
    if [[ -z "$reserved_memory_gb" ]]; then
        if [[ $total_memory_gb -le 8 ]]; then
            reserved_memory_gb=2
        elif [[ $total_memory_gb -le 32 ]]; then
            reserved_memory_gb=4
        elif [[ $total_memory_gb -le 64 ]]; then
            reserved_memory_gb=8
        else
            reserved_memory_gb=12
        fi
    fi

    # Calculate usable memory
    local usable_memory_gb=$((total_memory_gb - reserved_memory_gb))

    # Safety check
    if [[ $usable_memory_gb -lt $memory_per_process_gb ]]; then
        log_warn "Not enough usable memory, using minimal configuration"
        usable_memory_gb=$memory_per_process_gb
    fi

    # Calculate max processes based on constraints
    local max_processes_by_memory=$((usable_memory_gb / memory_per_process_gb))
    local max_processes_by_cpu=$usable_cpu_cores

    # Take the minimum (most restrictive constraint)
    local max_processes=$max_processes_by_memory
    if [[ $max_processes -gt $max_processes_by_cpu ]]; then
        max_processes=$max_processes_by_cpu
        log_debug "Limited by CPU cores: using $max_processes instead of $max_processes_by_memory"
    else
        log_debug "Limited by memory: using $max_processes processes"
    fi

    # Ensure at least 1 process
    if [[ $max_processes -lt 1 ]]; then
        max_processes=1
    fi

    log_info "System: ${total_memory_gb}GB RAM, ${total_cpu_cores} vCPUs"
    log_info "Usable: ${usable_memory_gb}GB RAM (${reserved_memory_gb}GB reserved), ${usable_cpu_cores} vCPUs (${cpu_usage_percent}%)"
    log_info "Constraints: memory allows ${max_processes_by_memory}, CPU allows ${max_processes_by_cpu}"
    log_info "Using ${max_processes} processes (${memory_per_process_gb}GB each)"

    echo "$max_processes"
}
