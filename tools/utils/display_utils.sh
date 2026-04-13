#!/usr/bin/env bash
set -eou pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT_DIR="${ROOT_DIR%/tools*}"
. "$ROOT_DIR/tools/utils/import.sh"

import utils/common_func.sh
import utils/logger.sh
import utils/color_codes.sh

# Function to create a line that fills the terminal width
create_line() {
    local char="${1:-=}"
    local width
    width=$(tput cols 2>/dev/null || echo 80)
    printf "%*s\n" "$width" "" | tr ' ' "$char"
}

# Function to center text within terminal width
center_text() {
    local text="$1"
    local width
    width=$(tput cols 2>/dev/null || echo 80)
    local text_length=${#text}
    local padding=$(( (width - text_length) / 2 ))
    printf "%*s%s\n" "$padding" "" "$text"
}

print_header() {
    local title="${1:-PROCESS STARTING}"
    local color="${2:-${BLUE}}"

    echo
    echo -e "${color}$(create_line)${NC}"
    echo -e "${color}$(center_text "$title")${NC}"
    echo -e "${color}$(create_line)${NC}"
    echo
}

print_footer() {
    local title="${1:-PROCESS COMPLETED SUCCESSFULLY}"
    local duration="${2:-}"
    local color="${3:-${BLUE}}"

    echo
    echo -e "${color}$(create_line)${NC}"
    echo -e "${color}$(center_text "$title")${NC}"
    if [[ -n "$duration" ]]; then
        echo -e "${color}$(center_text "Total time: ${duration}s")${NC}"
    fi
    echo -e "${color}$(create_line)${NC}"
    echo
}

# Simple print — clean output without timestamps or log-level prefixes.
# Use log_* when you want structured logging; use print for plain user-facing text.
#
# Usage:
#   print "hello"      # plain output
#   print              # blank line
print() {
    echo -e "$*"
}

print_error() {
    local title="${1:-PROCESS FAILED}"
    local color="${2:-${RED}}"

    echo
    echo -e "${color}$(create_line)${NC}"
    echo -e "${color}$(center_text "$title")${NC}"
    echo -e "${color}$(create_line)${NC}"
    echo
}
