include_guard(GLOBAL)

include(GNUInstallDirs)
include(ExternalProject)

function(external_configure_zlib _EP_BASE _EP_BUILD_DIR _INSTALL_DIR _STATIC_LIB _SHARED_LIB)
  set(DOWNLOAD_OPTIONS
    GIT_REPOSITORY "https://github.com/madler/zlib.git"
    GIT_TAG "${ZLIB_VERSION}"
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED FALSE)

  set(_BYPRODUCTS "")
  list(APPEND _BYPRODUCTS "${_SHARED_LIB}")
  list(APPEND _BYPRODUCTS "${_STATIC_LIB}")

  ExternalProject_Add(
    ${_EP_BASE}
    PREFIX ${_EP_BUILD_DIR}
    ${DOWNLOAD_OPTIONS}
    EXCLUDE_FROM_ALL ON
    INSTALL_DIR ${_INSTALL_DIR}
    CMAKE_ARGS
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DCMAKE_BUILD_TYPE=${AIRMETTLE_AIRTREE_DEPS_BUILD_TYPE}
    -DBUILD_SHARED_LIBS=OFF
    -DCMAKE_INSTALL_PREFIX=${_INSTALL_DIR}
    -DBUILD_SHARED_LIBS=OFF
    INSTALL_BYPRODUCTS ${_BYPRODUCTS}
    BUILD_COMMAND cmake --build . --config ${AIRMETTLE_AIRTREE_DEPS_BUILD_TYPE} -- -j${NPROC}
    INSTALL_COMMAND cmake --build . --target install
  )

  file(MAKE_DIRECTORY "${_INSTALL_DIR}/include")
endfunction()

function(configure_zlib)
  set(_DEPS_DIR "${AIRMETTLE_AIRTREE_DEPENDENCY_DIR}")
  set(_EP_BASE "zlib_ep")
  set(_EP_BUILD_DIR "${_DEPS_DIR}/${_EP_BASE}/build")

  airtree_dep_try_cache(DEP_NAME zlib EP_DIR "${_DEPS_DIR}/${_EP_BASE}" CACHE_HIT _cache_hit INSTALL_DIR _INSTALL_DIR)

  set(_SHARED_LIB "${_INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}z${CMAKE_SHARED_LIBRARY_SUFFIX}")
  set(_STATIC_LIB "${_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}z${CMAKE_STATIC_LIBRARY_SUFFIX}")

  if(_cache_hit)
    message(STATUS "${_EP_BASE} restored from cache.")
    add_custom_target(${_EP_BASE})
  elseif(NOT (EXISTS "${_SHARED_LIB}" OR EXISTS "${_STATIC_LIB}"))
    message(STATUS "${_EP_BASE} not found at ${_INSTALL_DIR}. Will download and build it.")
    external_configure_zlib(${_EP_BASE} ${_EP_BUILD_DIR} ${_INSTALL_DIR} ${_STATIC_LIB} ${_SHARED_LIB})
    add_custom_target(clean_zlib_build ALL
      COMMAND ${CMAKE_COMMAND} -E remove_directory ${_EP_BUILD_DIR}
      COMMENT "Cleaning up zlib build directory after installation"
    )
    add_dependencies(clean_zlib_build ${_EP_BASE})
    airtree_dep_mark_built(EP_TARGET ${_EP_BASE} INSTALL_DIR "${_INSTALL_DIR}")
  else()
    message(STATUS "${_EP_BASE} found at ${_INSTALL_DIR}. Skipping download and build.")
    add_custom_target(${_EP_BASE})
  endif()

  add_library(zlib::zlib_shared SHARED IMPORTED GLOBAL)
  set_target_properties(zlib::zlib_shared PROPERTIES IMPORTED_LOCATION "${_SHARED_LIB}")
  target_include_directories(zlib::zlib_shared SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  add_dependencies(zlib::zlib_shared ${_EP_BASE})

  add_dependencies(am_airtree_dependencies zlib::zlib_shared)

  add_library(zlib::zlib_static STATIC IMPORTED GLOBAL)
  set_target_properties(zlib::zlib_static PROPERTIES IMPORTED_LOCATION "${_STATIC_LIB}")
  target_include_directories(zlib::zlib_static SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  add_dependencies(zlib::zlib_static ${_EP_BASE})

  add_dependencies(am_airtree_dependencies zlib::zlib_static)

endfunction()

configure_zlib()

if (AIRMETTLE_AIRTREE_USE_SHARED_LIBS)
  add_library(zlib::zlib ALIAS zlib::zlib_shared)
else ()
  add_library(zlib::zlib ALIAS zlib::zlib_static)
endif ()
