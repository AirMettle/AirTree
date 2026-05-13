include_guard(GLOBAL)

include(GNUInstallDirs)
include(ExternalProject)

function(external_configure_zstd _EP_BASE _EP_BUILD_DIR _INSTALL_DIR _STATIC_LIB _SHARED_LIB)
  set(DOWNLOAD_OPTIONS
    GIT_REPOSITORY "https://github.com/facebook/zstd.git"
    GIT_TAG "${ZSTD_VERSION}"
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED FALSE)

  set(_BYPRODUCTS "")
  list(APPEND _BYPRODUCTS "${_SHARED_LIB}")
  list(APPEND _BYPRODUCTS "${_STATIC_LIB}")

  if(AIRMETTLE_AIRTREE_USE_SHARED_LIBS)
    set(_ZSTD_BUILD_SHARED_LIBS ON)
    set(_ZSTD_BUILD_STATIC_LIBS OFF)
    set(ZSTD_BUILD_STATIC OFF)
    set(ZSTD_BUILD_SHARED ON)
  else()
    set(_ZSTD_BUILD_SHARED_LIBS OFF)
    set(_ZSTD_BUILD_STATIC_LIBS ON)
    set(ZSTD_BUILD_STATIC ON)
    set(ZSTD_BUILD_SHARED OFF)
  endif()

  ExternalProject_Add(
    ${_EP_BASE}
    PREFIX ${_EP_BUILD_DIR}
    ${DOWNLOAD_OPTIONS}
    EXCLUDE_FROM_ALL ON
    INSTALL_DIR ${_INSTALL_DIR}
    SOURCE_SUBDIR build/cmake
    CMAKE_ARGS
    -DCMAKE_INSTALL_LIBDIR=lib
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DCMAKE_BUILD_TYPE=${AIRMETTLE_AIRTREE_DEPS_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=${_INSTALL_DIR}
    -DBUILD_SHARED_LIBS=${_ZSTD_BUILD_SHARED_LIBS}
    -DBUILD_STATIC_LIBS=${_ZSTD_BUILD_STATIC_LIBS}
    -DZSTD_BUILD_STATIC=${ZSTD_BUILD_STATIC}
    -DZSTD_BUILD_SHARED=${ZSTD_BUILD_SHARED}
    INSTALL_BYPRODUCTS ${_BYPRODUCTS}
    BUILD_COMMAND cmake --build . --config ${AIRMETTLE_AIRTREE_DEPS_BUILD_TYPE} -- -j${NPROC}
    INSTALL_COMMAND cmake --build . --target install
  )

  file(MAKE_DIRECTORY "${_INSTALL_DIR}/include")
endfunction()

function(configure_zstd)
  set(_DEPS_DIR "${AIRMETTLE_AIRTREE_DEPENDENCY_DIR}")
  set(_EP_BASE "zstd_ep")
  set(_EP_BUILD_DIR "${_DEPS_DIR}/${_EP_BASE}/build")

  airtree_dep_try_cache(DEP_NAME zstd EP_DIR "${_DEPS_DIR}/${_EP_BASE}" CACHE_HIT _cache_hit INSTALL_DIR _INSTALL_DIR)

  if(WIN32)
    set(_SHARED_LIB "${_INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}zstd${CMAKE_SHARED_LIBRARY_SUFFIX}")
    set(_STATIC_LIB "${_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}zstd_static${CMAKE_STATIC_LIBRARY_SUFFIX}")
  else()
    set(_SHARED_LIB "${_INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}zstd${CMAKE_SHARED_LIBRARY_SUFFIX}")
    set(_STATIC_LIB "${_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}zstd${CMAKE_STATIC_LIBRARY_SUFFIX}")
  endif()

  if(_cache_hit)
    message(STATUS "${_EP_BASE} restored from cache.")
    add_custom_target(${_EP_BASE})
  elseif(NOT (EXISTS "${_SHARED_LIB}" OR EXISTS "${_STATIC_LIB}"))
    message(STATUS "${_EP_BASE} not found at ${_INSTALL_DIR}. Will download and build it.")
    external_configure_zstd(${_EP_BASE} ${_EP_BUILD_DIR} ${_INSTALL_DIR} ${_STATIC_LIB} ${_SHARED_LIB})
    add_custom_target(clean_zstd_build ALL
      COMMAND ${CMAKE_COMMAND} -E remove_directory ${_EP_BUILD_DIR}
      COMMENT "Cleaning up zstd build directory after installation"
    )
    add_dependencies(clean_zstd_build ${_EP_BASE})
    airtree_dep_mark_built(EP_TARGET ${_EP_BASE} INSTALL_DIR "${_INSTALL_DIR}")
  else()
    message(STATUS "${_EP_BASE} found at ${_INSTALL_DIR}. Skipping download and build.")
    add_custom_target(${_EP_BASE}) # Dummy target to keep dependencies working
  endif()

  add_library(zstd::zstd_shared SHARED IMPORTED GLOBAL)
  set_target_properties(zstd::zstd_shared PROPERTIES IMPORTED_LOCATION "${_SHARED_LIB}")
  target_include_directories(zstd::zstd_shared SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  add_dependencies(zstd::zstd_shared ${_EP_BASE})

  add_dependencies(am_airtree_dependencies zstd::zstd_shared)

  add_library(zstd::zstd_static STATIC IMPORTED GLOBAL)
  set_target_properties(zstd::zstd_static PROPERTIES IMPORTED_LOCATION "${_STATIC_LIB}")
  target_include_directories(zstd::zstd_static SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  add_dependencies(zstd::zstd_static ${_EP_BASE})

  add_dependencies(am_airtree_dependencies zstd::zstd_static)

endfunction()

configure_zstd()

if (AIRMETTLE_AIRTREE_USE_SHARED_LIBS)
  add_library(zstd::zstd ALIAS zstd::zstd_shared)
else ()
  add_library(zstd::zstd ALIAS zstd::zstd_static)
endif ()
