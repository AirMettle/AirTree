#! /usr/bin/env bash

# Log level configuration
# Levels: 0=DEBUG, 1=INFO, 2=SUCCESS, 3=WARNING, 4=ERROR, 5=SILENT
declare -g LOG_LEVEL="${LOG_LEVEL:-1}"  # Default to INFO level

# Log level constants (for readability)
declare -gr LOG_DEBUG=0
declare -gr LOG_INFO=1
declare -gr LOG_SUCCESS=2
declare -gr LOG_WARNING=3
declare -gr LOG_ERROR=4
declare -gr LOG_SILENT=5

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT_DIR="${ROOT_DIR%/tools*}"
. "$ROOT_DIR/tools/utils/import.sh"

import utils/color_codes.sh

# Enhanced logging function with level control
log() {
    local level="$1"
    shift
    local level_num

    # Map level to number
    case "$level" in
        "DEBUG") level_num=0 ;;
        "INFO") level_num=1 ;;
        "SUCCESS") level_num=2 ;;
        "WARNING") level_num=3 ;;
        "ERROR") level_num=4 ;;
        "CLEANUP") level_num=1 ;;
        *) level_num=1 ;;
    esac

    # Only log if current level is >= minimum log level
    if [[ $level_num -ge $LOG_LEVEL ]]; then
        local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
        case "$level" in
            "INFO") echo -e "${BLUE}[$timestamp] INFO: $*${NC}" ;;
            "SUCCESS") echo -e "${GREEN}[$timestamp] SUCCESS: $*${NC}" ;;
            "WARNING") echo -e "${YELLOW}[$timestamp] WARNING: $*${NC}" >&2 ;;
            "ERROR") echo -e "${RED}[$timestamp] ERROR: $*${NC}" >&2 ;;
            "CLEANUP") echo -e "${CYAN}[$timestamp] CLEANUP: $*${NC}" ;;
            "DEBUG") echo -e "${NC}[$timestamp] DEBUG: $*${NC}" ;;
        esac
    fi
}

# Convenience functions remain the same
log_info() { log "INFO" "$@"; }
log_success() { log "SUCCESS" "$@"; }
log_warn() { log "WARNING" "$@"; }
log_error() { log "ERROR" "$@"; }
log_cleanup() { log "CLEANUP" "$@"; }
log_debug() { log "DEBUG" "$@"; }
