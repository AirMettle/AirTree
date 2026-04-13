find_program(_GCC9
    NAMES gcc-9 gcc
    HINTS /opt/rh/gcc-toolset-9/root/bin/
    NO_CACHE
    REQUIRED)
mark_as_advanced(_GCC9)
find_program(_GXX9
    NAMES g++-9 g++
    HINTS /opt/rh/gcc-toolset-9/root/bin/
    NO_CACHE
    REQUIRED)
mark_as_advanced(_GXX9)

if (NOT _GCC9)
  message(FATAL_ERROR "gcc 9 not found")
else ()
  execute_process(COMMAND ${_GCC9} -dumpversion OUTPUT_VARIABLE _GCC_MAJOR_VERSION)
  if (NOT _GCC_MAJOR_VERSION EQUAL "9")
    message(FATAL_ERROR "gcc 9 not found")
  else ()
    set(CMAKE_C_COMPILER "${_GCC9}" CACHE STRING "C Compiler" FORCE)
  endif ()
endif ()

if (NOT _GXX9)
  message(FATAL_ERROR "g++ 9 not found")
else ()
  execute_process(COMMAND ${_GXX9} -dumpversion OUTPUT_VARIABLE _GXX_MAJOR_VERSION)
  if (NOT _GXX_MAJOR_VERSION EQUAL "9")
    message(FATAL_ERROR "g++ 9 not found")
  else ()
    set(CMAKE_CXX_COMPILER "${_GXX9}" CACHE STRING "C Compiler" FORCE)
  endif ()
endif ()
