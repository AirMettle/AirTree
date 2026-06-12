find_program(_CLANG20
    NAMES clang-20 clang
    HINTS /opt/homebrew/opt/llvm@20/bin /usr/local/opt/llvm@20/bin
    NO_CACHE
    REQUIRED)
mark_as_advanced(_CLANG20)
find_program(_CLANGXX20
    NAMES clang++-20 clang++
    HINTS /opt/homebrew/opt/llvm@20/bin /usr/local/opt/llvm@20/bin
    NO_CACHE
    REQUIRED)
mark_as_advanced(_CLANGXX20)

if (NOT _CLANG20)
  message(FATAL_ERROR "clang 20 not found")
else ()
  execute_process(COMMAND ${_CLANG20} -dumpversion OUTPUT_VARIABLE _CLANG_MAJOR_VERSION)
  if (NOT _CLANG_MAJOR_VERSION MATCHES "^20")
    message(FATAL_ERROR "clang 20 not found")
  else ()
    set(CMAKE_C_COMPILER "${_CLANG20}" CACHE STRING "C Compiler" FORCE)
  endif ()
endif ()

if (NOT _CLANGXX20)
  message(FATAL_ERROR "clang++ 20 not found")
else ()
  execute_process(COMMAND ${_CLANGXX20} -dumpversion OUTPUT_VARIABLE _CLANGXX_MAJOR_VERSION)
  if (NOT _CLANGXX_MAJOR_VERSION MATCHES "^20")
    message(FATAL_ERROR "clang++ 20 not found")
  else ()
    set(CMAKE_CXX_COMPILER "${_CLANGXX20}" CACHE STRING "C Compiler" FORCE)
  endif ()
endif ()
