#! /usr/bin/env bash


set -eou pipefail


# A set of common functions and variables for bash scripts in the AirTree project. If you need something
# do check if it is already implemented here. If not, feel free to add it.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT_DIR="${ROOT_DIR%/tools*}"
. "$ROOT_DIR/tools/utils/import.sh"

import utils/logger.sh

# Utility: resolve the directory of the calling script.
# Kept as a general-purpose helper; individual scripts no longer need it for boilerplate.
_get_current_script_path(){
    local script_source="${BASH_SOURCE[0]:-$0}"
    if [[ -n "$script_source" ]]; then
        echo "$(cd "$(dirname "$script_source")" && pwd)"
    elif [[ -n "$0" ]]; then
        echo "$(dirname "$(readlink -f "$0" 2>/dev/null || realpath "$0" 2>/dev/null || echo "$0")")"
    else
        echo "$PWD" # Last resort fallback
    fi
}


run_step() {
    local allow_fail=false
    local timeout_seconds=""
    local max_retries=0
    local retry_delay=1

    # Parse options
    while [[ $# -gt 0 ]]; do
        case $1 in
            --allow-fail) allow_fail=true; shift ;;
            --timeout) timeout_seconds="$2"; shift 2 ;;
            --retries) max_retries="$2"; shift 2 ;;
            --retry-delay) retry_delay="$2"; shift 2 ;;
            *) break ;;
        esac
    done

    local heading="$1"
    shift
    local cmd=("$@")

    # Validate inputs
    if [[ -z "$heading" ]]; then
        log_error "run_step: heading is required"
        return 1
    fi
    if [[ ${#cmd[@]} -eq 0 ]]; then
        log_error "run_step: command is required"
        return 1
    fi

    # Caller info setup
    local caller_script="${BASH_SOURCE[1]##*/}"
    local caller_func="${FUNCNAME[1]:-MAIN}"
    local caller_line="${BASH_LINENO[0]}"

    # Retry loop
    local attempt=1
    while [[ $attempt -le $((max_retries + 1)) ]]; do
        if [[ $attempt -gt 1 ]]; then
            log_warn "Retry attempt $((attempt-1))/$max_retries for: $heading"
            sleep "$retry_delay"
        fi

        log_info "[ RUNNING ] $heading (attempt: $attempt, caller: ${caller_script}:${caller_func}:${caller_line})"
        log_debug "Command: ${cmd[*]}"

        # Prepare command with optional timeout
        local full_cmd=("${cmd[@]}")
        if [[ -n "$timeout_seconds" ]]; then
            full_cmd=("timeout" "$timeout_seconds" "${cmd[@]}")
            log_debug "Using timeout: ${timeout_seconds}s"
        fi

        local start_time=$(date +%s)

        # Execute command with timeout support
        {
            "${full_cmd[@]}" \
            > >(sed -u $'s/^/\x1b[0;37m/; s/$/\x1b[0m/') \
            2> >(sed -u $'s/^/\x1b[0;37m/; s/$/\x1b[0m/' >&2)
        }

        local exit_code=$?
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))

        # Handle successful execution
        if [ $exit_code -eq 0 ]; then
            local success_msg="[ OK ] $heading"
            if [[ $attempt -gt 1 ]]; then
                success_msg="$success_msg (succeeded on attempt $attempt after $((attempt-1)) retries)"
            fi
            success_msg="$success_msg (duration: ${duration}s)"
            log_success "$success_msg"
            return 0
        fi

        # Handle timeout specifically
        if [ $exit_code -eq 124 ] && [[ -n "$timeout_seconds" ]]; then
            local timeout_msg="$heading timed out after ${timeout_seconds}s"
            if [[ $attempt -lt $((max_retries + 1)) ]]; then
                log_warn "$timeout_msg (attempt $attempt/$((max_retries + 1)))"
            else
                log_error "$timeout_msg after $((max_retries + 1)) attempts"
                if [ "$allow_fail" = true ]; then
                    log_warn "Continuing despite timeout failure"
                    return 0
                else
                    exit $exit_code
                fi
            fi
        # Handle other failures
        else
            local failure_msg="$heading failed with exit code $exit_code (duration: ${duration}s)"
            if [[ $attempt -lt $((max_retries + 1)) ]]; then
                log_warn "$failure_msg (attempt $attempt/$((max_retries + 1)))"
            else
                # This was the last attempt, handle final failure
                if [[ $max_retries -gt 0 ]]; then
                    failure_msg="$heading failed after $((max_retries + 1)) attempts (final exit code: $exit_code)"
                fi

                if [ "$allow_fail" = true ]; then
                    log_warn "$failure_msg - continuing"
                    return 0
                else
                    log_error "$failure_msg"
                    exit $exit_code
                fi
            fi
        fi

        ((attempt++))
    done
}

# Function to find repo root
get_project_root() {
    log_debug "Searching for project root directory"

    # Method 1: Try git first
    if command -v git >/dev/null 2>&1; then
        log_debug "Git command available, checking if inside git repository"
        if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
            log_debug "Inside git repository, getting toplevel directory"
            local git_root
            git_root=$(git rev-parse --show-toplevel 2>/dev/null)
            if [[ $? -eq 0 && -n "$git_root" ]]; then
                log_debug "Found git root: $git_root"
                echo "$git_root"
                return 0
            fi
        fi
    fi

    # Method 2: Look for marker files
    log_debug "Git method failed, searching for marker files"
    local dir="$PWD"
    local markers=(".git" "Makefile" "CMakeLists.txt" "README.md" "package.json")

    while [[ "$dir" != "/" ]]; do
        log_debug "Checking directory: $dir"
        for marker in "${markers[@]}"; do
            if [[ -e "$dir/$marker" ]]; then
                log_debug "Found marker file '$marker' in: $dir"
                echo "$dir"
                return 0
            fi
        done
        dir=$(dirname "$dir")
    done

    log_debug "Marker file method failed, trying script location fallback"

    # Method 3: Fall back to script location if we know the structure
    if [[ -n "${BASH_SOURCE[0]:-}" ]]; then
        local script_dir
        script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
        local fallback_root
        fallback_root="$(cd "$script_dir/../../" && pwd)"

        log_debug "Script-based fallback root: $fallback_root"

        # Validate the fallback actually looks like a project root
        if [[ -d "$fallback_root" ]]; then
            for marker in "${markers[@]}"; do
                if [[ -e "$fallback_root/$marker" ]]; then
                    log_debug "Validated fallback root with marker: $marker"
                    echo "$fallback_root"
                    return 0
                fi
            done
        fi
    fi

    log_error "Could not determine project root directory"
    return 1
}


# Error handler
error_exit() {
    log_error "$1"
    exit 1
}
