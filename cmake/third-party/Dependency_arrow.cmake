include_guard(GLOBAL)

include(GNUInstallDirs)
include(ExternalProject)

function(external_configure_arrow _EP_BASE _EP_BUILD_DIR _INSTALL_DIR  _ARROW_SHARED_LIB _ARROW_STATIC_LIB _PARQUET_SHARED_LIB _PARQUET_STATIC_LIB _ARROW_BUNDLED_SHARED_LIB _ARROW_BUNDLED_STATIC_LIB)
  # Remote
  set(DOWNLOAD_OPTIONS
    GIT_REPOSITORY "https://github.com/apache/arrow.git"
    GIT_TAG "${ARROW_VERSION}"
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED FALSE)

  set(_BYPRODUCTS "")
  list(APPEND _BYPRODUCTS "${_ARROW_SHARED_LIB}")
  list(APPEND _BYPRODUCTS "${_ARROW_STATIC_LIB}")
  list(APPEND _BYPRODUCTS "${_PARQUET_SHARED_LIB}")
  list(APPEND _BYPRODUCTS "${_PARQUET_STATIC_LIB}")
  list(APPEND _BYPRODUCTS "${_ARROW_BUNDLED_SHARED_LIB}")
  list(APPEND _BYPRODUCTS "${_ARROW_BUNDLED_STATIC_LIB}")

  set(ARROW_EXTRA_ARGS "")
  if(WIN32)
      list(APPEND ARROW_EXTRA_ARGS "-DCMAKE_SYSTEM_PROCESSOR=AMD64")
  endif()

  if(AIRMETTLE_AIRTREE_USE_SHARED_LIBS)
      set(ARROW_BUILD_SHARED ON)
      set(ARROW_BUILD_STATIC OFF)
      set(ARROW_DEP_SHARED ON)
      set(ARROW_C_FLAGS "")
      set(ARROW_CXX_FLAGS "")
  else()
      set(ARROW_BUILD_SHARED OFF)
      set(ARROW_BUILD_STATIC ON)
      set(ARROW_DEP_SHARED OFF)
      set(ARROW_C_FLAGS "-DZLIB_STATIC")
      set(ARROW_CXX_FLAGS "-DZLIB_STATIC")
  endif()

  ExternalProject_Add(
    ${_EP_BASE}
    PREFIX ${_EP_BUILD_DIR}
    ${DOWNLOAD_OPTIONS}
    EXCLUDE_FROM_ALL ON
    INSTALL_DIR ${_INSTALL_DIR}
    GIT_SUBMODULES_RECURSE 1
    SOURCE_DIR ${_EP_BUILD_DIR}/build/src/${_EP_BASE}  # Root directory of Arrow repo
    SOURCE_SUBDIR "cpp"  # Specify the 'cpp' subdirectory
    CMAKE_ARGS
    ${ARROW_EXTRA_ARGS}
    -DCMAKE_INSTALL_LIBDIR=lib
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DCMAKE_BUILD_TYPE=${AIRMETTLE_AIRTREE_DEPS_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=${_INSTALL_DIR}
    -DARROW_DEPENDENCY_SOURCE=BUNDLED
    -DVCPKG_TARGET_TRIPLET=x64-windows-static
    -DARROW_BUILD_STATIC=${ARROW_BUILD_STATIC}
    -DARROW_BUILD_SHARED=${ARROW_BUILD_SHARED}
    -DARROW_DEPENDENCY_USE_SHARED=${ARROW_DEP_SHARED}
    -DCMAKE_C_FLAGS=${ARROW_C_FLAGS}
    -DCMAKE_CXX_FLAGS=${ARROW_CXX_FLAGS}
    -DARROW_WITH_RE2=OFF
    -DARROW_BUILD_TESTS=OFF
    -DARROW_WITH_BZ2=OFF
    -DARROW_WITH_ZLIB=ON
    -DARROW_WITH_ZSTD=ON
    -DARROW_WITH_LZ4=OFF
    -DARROW_WITH_SNAPPY=ON
    -DSnappy_SOURCE=BUNDLED
    -DThrift_SOURCE=BUNDLED
    -DARROW_WITH_UTF8PROC=OFF
    -DARROW_DATASET=ON
    -DARROW_PARQUET=ON
    -DARROW_CSV=ON
    -DARROW_GANDIVA=OFF
    -DARROW_ACERO=OFF
    -DARROW_MIMALLOC=OFF
    INSTALL_BYPRODUCTS ${_BYPRODUCTS}
    BUILD_COMMAND cmake --build . --config ${AIRMETTLE_AIRTREE_DEPS_BUILD_TYPE} -- -j${NPROC}
    INSTALL_COMMAND cmake --build . --target install
  )
  
  file(MAKE_DIRECTORY "${_INSTALL_DIR}/include")
endfunction()

function(configure_arrow)

  set(_DEPS_DIR "${AIRMETTLE_AIRTREE_DEPENDENCY_DIR}")
  set(_EP_BASE "arrow_ep")
  set(_EP_BUILD_DIR "${_DEPS_DIR}/${_EP_BASE}/build")

  airtree_dep_try_cache(DEP_NAME arrow EP_DIR "${_DEPS_DIR}/${_EP_BASE}" CACHE_HIT _cache_hit INSTALL_DIR _INSTALL_DIR)

  # Define expected install outputs
  if(WIN32)
    set(_ARROW_SHARED_LIB "${_INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}arrow${CMAKE_SHARED_LIBRARY_SUFFIX}")
    set(_ARROW_STATIC_LIB "${_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}arrow_static${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(_PARQUET_SHARED_LIB "${_INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}parquet${CMAKE_SHARED_LIBRARY_SUFFIX}")
    set(_PARQUET_STATIC_LIB "${_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}parquet_static${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(_ARROW_BUNDLED_SHARED_LIB "${_INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}arrow_bundled_dependencies${CMAKE_SHARED_LIBRARY_SUFFIX}")
    set(_ARROW_BUNDLED_STATIC_LIB "${_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}arrow_bundled_dependencies${CMAKE_STATIC_LIBRARY_SUFFIX}")
  else()
    set(_ARROW_SHARED_LIB "${_INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}arrow${CMAKE_SHARED_LIBRARY_SUFFIX}")
    set(_ARROW_STATIC_LIB "${_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}arrow${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(_PARQUET_SHARED_LIB "${_INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}parquet${CMAKE_SHARED_LIBRARY_SUFFIX}")
    set(_PARQUET_STATIC_LIB "${_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}parquet${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(_ARROW_BUNDLED_SHARED_LIB "${_INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}arrow_bundled_dependencies${CMAKE_SHARED_LIBRARY_SUFFIX}")
    set(_ARROW_BUNDLED_STATIC_LIB "${_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}arrow_bundled_dependencies${CMAKE_STATIC_LIBRARY_SUFFIX}")
  endif()

  if(_cache_hit)
    message(STATUS "${_EP_BASE} restored from cache.")
    add_custom_target(${_EP_BASE})
  elseif(NOT EXISTS "${_ARROW_SHARED_LIB}" OR NOT EXISTS "${_ARROW_STATIC_LIB}")
  # I've noticed that any aggressive writes on a windows system can cause file locks that prevent
  # CMake from properly cleaning up the build directory, which can lead to failed builds. To mitigate this,
  # we check for the existence of the build directory and remove it before starting a new build, ensuring a 
  # clean slate for the build process but we will not try to clean up the build directory after the build 
  # because of the same reason. Instead, we will just leave it there and let the next build clean it up if necessary.
    if(EXISTS "${_EP_BUILD_DIR}")
      message(STATUS "Previous build directory for ${_EP_BASE} found at ${_EP_BUILD_DIR}. Removing it to ensure a clean build.")
      file(REMOVE_RECURSE "${_EP_BUILD_DIR}")
    endif()
    message(STATUS "${_EP_BASE} not found at ${_INSTALL_DIR}. Will download and build it.")
    external_configure_arrow(${_EP_BASE} ${_EP_BUILD_DIR} ${_INSTALL_DIR} ${_ARROW_SHARED_LIB} ${_ARROW_STATIC_LIB} ${_PARQUET_SHARED_LIB} ${_PARQUET_STATIC_LIB} ${_ARROW_BUNDLED_SHARED_LIB} ${_ARROW_BUNDLED_STATIC_LIB})
    if (NOT WIN32)
      add_custom_target(clean_arrow_build ALL
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${_EP_BUILD_DIR}
        COMMENT "Cleaning up arrow_build directory after installation"
      )
      add_dependencies(clean_arrow_build ${_EP_BASE})
    endif()
    airtree_dep_mark_built(EP_TARGET ${_EP_BASE} INSTALL_DIR "${_INSTALL_DIR}")
  else()
    message(STATUS "${_EP_BASE} found at ${_INSTALL_DIR}. Skipping download and build.")
    add_custom_target(${_EP_BASE}) # Dummy target to keep dependencies working
  endif()

  set(_DEPS_SHARED "")
  list(APPEND _DEPS_SHARED zstd::zstd zlib::zlib)

  # For static builds, Arrow often requires linking to additional sub-libraries
  set(_DEPS_STATIC "")
  list(APPEND _DEPS_STATIC zstd::zstd zlib::zlib)
  list(APPEND _DEPS_STATIC arrow::arrow_static parquet::parquet_static)

  # IMPORTED TARGET: arrow::bundled_shared
  add_library(arrow::bundled_shared SHARED IMPORTED GLOBAL)
  set_target_properties(arrow::bundled_shared PROPERTIES IMPORTED_LOCATION "${_ARROW_BUNDLED_SHARED_LIB}")
  add_dependencies(arrow::bundled_shared ${_EP_BASE})

  add_dependencies(am_airtree_dependencies arrow::bundled_shared)

  # IMPORTED TARGET: arrow::bundled_static
  add_library(arrow::bundled_static STATIC IMPORTED GLOBAL)
  set_target_properties(arrow::bundled_static PROPERTIES IMPORTED_LOCATION "${_ARROW_BUNDLED_STATIC_LIB}")
  add_dependencies(arrow::bundled_static ${_EP_BASE})

  add_dependencies(am_airtree_dependencies arrow::bundled_static)

  # IMPORTED TARGET: arrow::arrow_shared
  add_library(arrow::arrow_shared SHARED IMPORTED GLOBAL)
  set_target_properties(arrow::arrow_shared PROPERTIES IMPORTED_LOCATION "${_ARROW_SHARED_LIB}")
  target_include_directories(arrow::arrow_shared SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  target_link_libraries(arrow::arrow_shared INTERFACE ${_DEPS_SHARED} arrow::bundled_shared)
  add_dependencies(arrow::arrow_shared ${_EP_BASE})

  add_dependencies(am_airtree_dependencies arrow::arrow_shared)

  # IMPORTED TARGET: arrow::arrow_static
  add_library(arrow::arrow_static STATIC IMPORTED GLOBAL)
  set_target_properties(arrow::arrow_static PROPERTIES IMPORTED_LOCATION "${_ARROW_STATIC_LIB}")
  target_include_directories(arrow::arrow_static SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  target_link_libraries(arrow::arrow_static INTERFACE ${_DEPS_STATIC} arrow::bundled_static)
  add_dependencies(arrow::arrow_static ${_EP_BASE})
  target_compile_definitions(arrow::arrow_static INTERFACE ARROW_STATIC)

  add_dependencies(am_airtree_dependencies arrow::arrow_static)

  # IMPORTED TARGET: parquet::parquet_shared
  add_library(parquet::parquet_shared SHARED IMPORTED GLOBAL)
  set_target_properties(parquet::parquet_shared PROPERTIES IMPORTED_LOCATION "${_PARQUET_SHARED_LIB}")
  target_include_directories(parquet::parquet_shared SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  target_link_libraries(parquet::parquet_shared INTERFACE ${_DEPS_SHARED} arrow::bundled_shared)
  add_dependencies(parquet::parquet_shared ${_EP_BASE})

  add_dependencies(am_airtree_dependencies parquet::parquet_shared)

  # IMPORTED TARGET: parquet::parquet_static
  add_library(parquet::parquet_static STATIC IMPORTED GLOBAL)
  set_target_properties(parquet::parquet_static PROPERTIES IMPORTED_LOCATION "${_PARQUET_STATIC_LIB}")
  target_include_directories(parquet::parquet_static SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  target_link_libraries(parquet::parquet_static INTERFACE ${_DEPS_STATIC} arrow::bundled_static)
  add_dependencies(parquet::parquet_static ${_EP_BASE})
  target_compile_definitions(parquet::parquet_static INTERFACE PARQUET_STATIC)

  add_dependencies(am_airtree_dependencies parquet::parquet_static)

endfunction()

configure_arrow()

# Alias arrow::arrow, parquet::parquet to the shared or static lib we intend to use
if (AIRMETTLE_AIRTREE_USE_SHARED_LIBS)
  add_library(arrow::arrow ALIAS arrow::arrow_shared)
  add_library(parquet::parquet ALIAS parquet::parquet_shared)
else ()
  add_library(arrow::arrow ALIAS arrow::arrow_static)
  add_library(parquet::parquet ALIAS parquet::parquet_static)
endif ()
