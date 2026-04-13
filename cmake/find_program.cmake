function(find_python_executable)

    # Determine which Python to use - prioritize system Python over pyenv
    if(DEFINED ENV{VIRTUAL_ENV})
        set(PYTHON_EXECUTABLE "$ENV{VIRTUAL_ENV}/bin/python" PARENT_SCOPE)
        message(STATUS "Using Python from virtual environment: ${PYTHON_EXECUTABLE}")
    elseif(PYTHON_EXECUTABLE)
        message(STATUS "Using explicitly set Python: ${PYTHON_EXECUTABLE}")
    else()
        # First try to find system Python in standard locations (ignore pyenv)
        find_program(SYSTEM_PYTHON_EXECUTABLE
            NAMES python3.12
            PATHS /usr/bin /bin /usr/local/bin
            NO_DEFAULT_PATH
        )
        
        if(SYSTEM_PYTHON_EXECUTABLE)
            set(PYTHON_EXECUTABLE ${SYSTEM_PYTHON_EXECUTABLE} PARENT_SCOPE)
            message(STATUS "Found system Python: ${PYTHON_EXECUTABLE}")
            
            # Verify it works
            execute_process(
                COMMAND ${PYTHON_EXECUTABLE} --version
                OUTPUT_VARIABLE PYTHON_VERSION_OUTPUT
                ERROR_VARIABLE PYTHON_VERSION_ERROR
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_STRIP_TRAILING_WHITESPACE
            )
            message(STATUS "System Python version: ${PYTHON_VERSION_OUTPUT}")
        else()
            # Fallback to CMake's default Python finding (might find pyenv)
            find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
            set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE} PARENT_SCOPE)
            message(STATUS "Fallback - Found Python: ${PYTHON_EXECUTABLE}")
            
            execute_process(
                COMMAND ${PYTHON_EXECUTABLE} --version
                OUTPUT_VARIABLE PYTHON_VERSION_OUTPUT
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            message(STATUS "Fallback Python version: ${PYTHON_VERSION_OUTPUT}")
        endif()
    endif()

endfunction()
