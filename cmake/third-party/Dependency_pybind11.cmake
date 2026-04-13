include_guard(GLOBAL)

include(FetchContent)

find_python_executable()
message(STATUS "Using Python executable for pybind11: ${PYTHON_EXECUTABLE}")

# Ensure pybind11 finds the correct Python when added as a subdirectory
set(PYTHON_EXECUTABLE ${PYTHON_EXECUTABLE} CACHE FILEPATH "" FORCE)
set(Python3_EXECUTABLE ${PYTHON_EXECUTABLE})
set(Python_EXECUTABLE ${PYTHON_EXECUTABLE})

# Cache pybind11 source in the shared dependency directory alongside other deps
set(FETCHCONTENT_BASE_DIR "${AIRMETTLE_AIRTREE_DEPENDENCY_DIR}/pybind11_fc")

FetchContent_Declare(pybind11
  GIT_REPOSITORY "https://github.com/pybind/pybind11.git"
  GIT_TAG "${PYBIND11_VERSION}"
  GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(pybind11)

# Compute hash for DAG completeness (no S3 caching — FetchContent handles its own caching)
airtree_dep_compute_hash(
  DEP_NAME     "pybind11"
  DEP_VERSION  "${PYBIND11_VERSION}"
  RECIPE_FILE  "${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_pybind11.cmake"
  DEPENDS      ""
  OUT_VAR      _pybind11_hash
)
