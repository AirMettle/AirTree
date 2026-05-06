#!/usr/bin/env bash
set -eou pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT_DIR="${ROOT_DIR%/tools*}"
. "$ROOT_DIR/tools/utils/import.sh"
import utils/build_utils.sh
import utils/common_func.sh
import utils/logger.sh
import utils/color_codes.sh
import utils/display_utils.sh

OS_NAME="$(uname | awk '{ print tolower($0) }')"

# Generic test runner function - supports both ctest and pytest
# Generic test runner function - supports both ctest and pytest
run_tests() {
    local test_type="$1"           # "UNIT", "FUNCTIONAL", "PYTHON"
    local test_pattern_or_dirs="$2" # For ctest: "^ut_" or "^fn_", For pytest: space-separated directories
    local build_or_project_dir="$3" # For ctest: build directory, For pytest: project directory
    local num_cores_or_venv="${4:-}" # For ctest: num_cores, For python: venv_dir (optional)
    local venv_dir="${5:-}"        # For python tests when 5 args are passed

    print_header "STARTING ${test_type} TESTS"

    log_debug "Configuration:"
    log_debug "Test type: ${test_type}"
    log_debug "Project root: ${PROJECT_ROOT}"

    # START TIMER HERE - before calling sub-functions
    local start_time=$(date +%s)

    if [[ "$test_type" == "PYTHON" ]]; then
        # Determine venv directory based on arguments
        local python_venv_dir=""
        if [[ -n "$venv_dir" ]]; then
            # 5 arguments: run_tests "PYTHON" "test_dirs" "project_dir" "num_cores" "venv_dir"
            python_venv_dir="$venv_dir"
        elif [[ -n "$num_cores_or_venv" ]]; then
            # 4 arguments: run_tests "PYTHON" "test_dirs" "project_dir" "venv_dir"
            python_venv_dir="$num_cores_or_venv"
        else
            # 3 arguments: run_tests "PYTHON" "test_dirs" "project_dir" - use default
            python_venv_dir="$build_or_project_dir/venv"
        fi

        log_debug "Project directory: ${build_or_project_dir}"
        log_debug "Test directories: ${test_pattern_or_dirs}"
        log_debug "Virtual environment: ${python_venv_dir}"

        run_python_tests "$test_pattern_or_dirs" "$build_or_project_dir" "$python_venv_dir"
    else
        # For ctest, use num_cores (default if not provided)
        local num_cores="${num_cores_or_venv:-$(get_parallel_processes | tail -1)}"
        log_debug "Build directory: ${build_or_project_dir}"
        log_debug "Parallel jobs: ${num_cores}"
        log_debug "Test pattern: ${test_pattern_or_dirs}"

        run_ctest_tests "$test_type" "$test_pattern_or_dirs" "$build_or_project_dir" "$num_cores"
    fi

    local test_success=$?
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))

    if [[ $test_success -eq 0 ]]; then
        print_footer "${test_type} TESTS COMPLETED SUCCESSFULLY" "$duration"
        return 0
    else
        print_error "${test_type} TESTS FAILED"
        return 1
    fi
}

# Run ctest-based tests (existing functionality)
run_ctest_tests() {
    local test_type="$1"
    local test_pattern="$2"
    local build_dir="$3"
    local num_cores="$4"

    # REMOVE local start_time declaration from here

    # Change to build directory
    run_step "Changing to build directory" pushd "$build_dir" >/dev/null

    log_info "Running ${test_type,,} tests..."
    echo

    if [[ "$OS_NAME" == *"mingw"* || "$OS_NAME" == *"msys"* || "$OS_NAME" == *"cygwin"* ]]; then
        echo "Windows detected: Injecting dependency DLLs into PATH..."
        
        export PATH="/c/vcpkg/installed/x64-windows/bin:$PATH"
        
        DEPS_BIN_DIRS=$(find ${CMAKE_BUILD_DIR}/.airmettle/airtree-deps -type d -name "bin" | paste -sd ":" -)
        if [ -n "$DEPS_BIN_DIRS" ]; then
            export PATH="$DEPS_BIN_DIRS:$PATH"
        fi
    fi

    # Run tests with formatted output
    if ctest --output-on-failure --progress -R "$test_pattern" -j "$num_cores" 2>&1 | \
        format_ctest_output "$build_dir"; then
        run_step "Returning to original directory" popd >/dev/null
        return 0
    else
        run_step "Returning to original directory" popd >/dev/null
        return 1
    fi
}

# Run pytest-based tests with configurable venv directory
run_python_tests() {
    local test_dirs="$1"
    local project_dir="$2"
    local venv_dir="$3"

    # REMOVE local start_time declaration from here

    # Change to project directory
    run_step "Changing to project directory" pushd "$project_dir"

    # Check for virtual environment
    local activate_script="$venv_dir/bin/activate"
    if [[ ! -f "$activate_script" ]]; then
        log_error "Virtual environment activation script not found: $activate_script"
        log_error "Please create the virtual environment first"
        run_step "Returning to original directory" popd
        return 1
    fi

    log_info "Activating Python virtual environment: $venv_dir"
    source "$activate_script"

    # Verify Python is from the venv
    local python_path=$(python -c 'import sys; print(sys.executable)')
    log_debug "Using Python: $python_path"

    # Check and install pytest if needed
    if ! python -c "import pytest" &>/dev/null; then
        run_step "Installing pytest" pip install pytest numpy
    fi

    log_info "Running Python tests..."
    echo

    # Convert space-separated dirs to array
    local test_dirs_array=($test_dirs)

    # Run pytest with formatted output
    if python -B -m pytest -v -s "${test_dirs_array[@]}" 2>&1 | \
        format_pytest_output; then
        run_step "Returning to original directory" popd
        return 0
    else
        run_step "Returning to original directory" popd
        return 1
    fi
}

# Function to get test count from a Google Test executable
get_gtest_count() {
    local test_executable="$1"
    local build_dir="$2"

    # Find the executable in the build directory tree
    local exec_path=$(find "$build_dir" -type f -name "$test_executable" -executable 2>/dev/null | head -1)

    if [[ -n "$exec_path" && -x "$exec_path" ]]; then
        # Run with --gtest_list_tests and count test cases (lines starting with spaces)
        local count=$("$exec_path" --gtest_list_tests 2>/dev/null | grep -c "^  " || echo "")
        echo "$count"
    else
        echo ""
    fi
}

# Function to format ctest output with test case counts
format_ctest_output() {
    local build_dir="$1"
    declare -A test_counts

    while IFS= read -r line; do
        case "$line" in
            *"Test project"*)
                echo -e "${PURPLE}$line${NC}"
                ;;
            *"Start "*": "*)
                # Extract test name and get its count
                if [[ "$line" =~ Start[[:space:]]+[0-9]+:[[:space:]]+(.+) ]]; then
                    local test_name="${BASH_REMATCH[1]}"
                    # Get test count in background to not slow down display
                    local count=$(get_gtest_count "$test_name" "$build_dir")
                    if [[ -n "$count" && "$count" -gt 0 ]]; then
                        test_counts["$test_name"]="$count"
                    fi
                fi
                # Skip showing start lines entirely
                ;;
            *"Passed"*)
                cleaned_line=$(echo "$line" | sed 's/Test[[:space:]]\+#[0-9]\+:[[:space:]]*//')
                # Extract test name from the line (it's the second field after the progress indicator)
                local test_name=$(echo "$cleaned_line" | awk '{print $2}')
                if [[ -v "test_counts[$test_name]" ]] && [[ -n "${test_counts[$test_name]}" ]]; then
                    local count="${test_counts[$test_name]}"
                    # Insert test count before "Passed"
                    cleaned_line=$(echo "$cleaned_line" | sed "s/Passed/ [$count\/$count]  Passed/")
                fi
                echo -e "${GREEN} $cleaned_line${NC}"
                ;;
            *"Failed"*)
                cleaned_line=$(echo "$line" | sed 's/Test[[:space:]]\+#[0-9]\+:[[:space:]]*//')
                local test_name=$(echo "$cleaned_line" | awk '{print $2}')
                if [[ -v "test_counts[$test_name]" ]] && [[ -n "${test_counts[$test_name]}" ]]; then
                    local count="${test_counts[$test_name]}"
                    cleaned_line=$(echo "$cleaned_line" | sed "s/Failed/ [$count\/$count]  Failed/")
                fi
                echo -e "${RED} $cleaned_line${NC}"
                ;;
            *"tests passed"*)
                echo -e "${GREEN} $line${NC}"
                ;;
            *"tests failed"*)
                echo -e "${RED} $line${NC}"
                ;;
            *"DISABLED"*)
                echo -e "${YELLOW} $line${NC}"
                ;;
            *"Total Test time"*)
                # Skip showing ctest total time since we show our own
                ;;
            *"The following tests FAILED"*)
                echo -e "${RED} $line${NC}"
                ;;
            *"Errors while running CTest"*)
                echo -e "${RED} $line${NC}"
                ;;
            "")
                # Skip empty lines
                ;;
            *)
                # Default: print other lines (includes detailed failure output)
                echo -e "${NC}$line${NC}"
                ;;
        esac
    done
}

# Function to format pytest output (existing)
format_pytest_output() {
    while IFS= read -r line; do
        case "$line" in
            *"PASSED"*)
                echo -e "${GREEN}$line${NC}"
                ;;
            *"FAILED"*)
                echo -e "${RED}$line${NC}"
                ;;
            *"ERROR"*)
                echo -e "${RED}$line${NC}"
                ;;
            *"SKIPPED"*)
                echo -e "${YELLOW}$line${NC}"
                ;;
            *"collected"*)
                echo -e "${CYAN}$line${NC}"
                ;;
            *"="*"test session starts"*"="*)
                echo -e "${PURPLE}$line${NC}"
                ;;
            *"="*"FAILURES"*"="*)
                echo -e "${RED}$line${NC}"
                ;;
            *"="*"ERRORS"*"="*)
                echo -e "${RED}$line${NC}"
                ;;
            *"="*"short test summary"*"="*)
                echo -e "${YELLOW}$line${NC}"
                ;;
            *"="*" passed"*|*"="*" failed"*|*"="*" error"*)
                if [[ "$line" == *" failed"* ]] || [[ "$line" == *" error"* ]]; then
                    echo -e "${RED}$line${NC}"
                else
                    echo -e "${GREEN}$line${NC}"
                fi
                ;;
            "")
                # Skip empty lines
                ;;
            *)
                # Default: print other lines
                echo -e "${NC}$line${NC}"
                ;;
        esac
    done
}
