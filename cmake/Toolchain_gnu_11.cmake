find_program(_GCC11
    NAMES gcc-11 gcc
    HINTS /opt/rh/gcc-toolset-11/root/bin/
    NO_CACHE
    REQUIRED)
mark_as_advanced(_GCC11)
find_program(_GXX11
    NAMES g++-11 g++
    HINTS /opt/rh/gcc-toolset-11/root/bin/
    NO_CACHE
    REQUIRED)
mark_as_advanced(_GXX11)

if (NOT _GCC11)
  message(FATAL_ERROR "gcc 11 not found")
else ()
  execute_process(COMMAND ${_GCC11} -dumpversion OUTPUT_VARIABLE _GCC_MAJOR_VERSION)
  if (NOT _GCC_MAJOR_VERSION MATCHES "^11")
    message(FATAL_ERROR "gcc 11 not found")
  else ()
    set(CMAKE_C_COMPILER "${_GCC11}" CACHE STRING "C Compiler" FORCE)
  endif ()
endif ()

if (NOT _GXX11)
  message(FATAL_ERROR "g++ 11 not found")
else ()
  execute_process(COMMAND ${_GXX11} -dumpversion OUTPUT_VARIABLE _GXX_MAJOR_VERSION)
  if (NOT _GXX_MAJOR_VERSION MATCHES "^11")
    message(FATAL_ERROR "g++ 11 not found")
  else ()
    set(CMAKE_CXX_COMPILER "${_GXX11}" CACHE STRING "C Compiler" FORCE)
  endif ()
endif ()
