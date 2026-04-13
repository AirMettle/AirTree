include_guard(GLOBAL)

include(GNUInstallDirs)
include(ExternalProject)

function(external_configure_lz4 _EP_BASE _EP_BUILD_DIR _INSTALL_DIR _STATIC_LIB _SHARED_LIB)
  set(DOWNLOAD_OPTIONS
    GIT_REPOSITORY "https://github.com/lz4/lz4.git"
    GIT_TAG "${LZ4_VERSION}"
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
    CONFIGURE_COMMAND ""  # LZ4 does not use CMake for configuration
    BUILD_COMMAND make -C lib
    INSTALL_COMMAND make -C lib PREFIX=${_INSTALL_DIR} install
    INSTALL_BYPRODUCTS ${_BYPRODUCTS}
    BUILD_IN_SOURCE 1
  )

  file(MAKE_DIRECTORY "${_INSTALL_DIR}/include")
endfunction()

function(configure_lz4)
  set(_DEPS_DIR "${AIRMETTLE_AIRTREE_DEPENDENCY_DIR}")
  set(_EP_BASE "lz4_ep")
  set(_EP_BUILD_DIR "${_DEPS_DIR}/${_EP_BASE}/build")

  airtree_dep_try_cache(DEP_NAME lz4 EP_DIR "${_DEPS_DIR}/${_EP_BASE}" CACHE_HIT _cache_hit INSTALL_DIR _INSTALL_DIR)

  set(_SHARED_LIB "${_INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}lz4${CMAKE_SHARED_LIBRARY_SUFFIX}")
  set(_STATIC_LIB "${_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}lz4${CMAKE_STATIC_LIBRARY_SUFFIX}")

  if(_cache_hit)
    message(STATUS "${_EP_BASE} restored from cache.")
    add_custom_target(${_EP_BASE})
  elseif(NOT (EXISTS "${_SHARED_LIB}" OR EXISTS "${_STATIC_LIB}"))
    message(STATUS "${_EP_BASE} not found at ${_INSTALL_DIR}. Will download and build it.")
    external_configure_lz4(${_EP_BASE} ${_EP_BUILD_DIR} ${_INSTALL_DIR} ${_STATIC_LIB} ${_SHARED_LIB})
    add_custom_target(clean_lz4_build ALL
      COMMAND ${CMAKE_COMMAND} -E remove_directory ${_EP_BUILD_DIR}
      COMMENT "Cleaning up lz4 build directory after installation"
    )
    add_dependencies(clean_lz4_build ${_EP_BASE})
    airtree_dep_mark_built(EP_TARGET ${_EP_BASE} INSTALL_DIR "${_INSTALL_DIR}")
  else()
    message(STATUS "${_EP_BASE} found at ${_INSTALL_DIR}. Skipping download and build.")
    add_custom_target(${_EP_BASE}) # Dummy target to keep dependencies working
  endif()

  add_library(lz4::lz4_shared SHARED IMPORTED GLOBAL)
  set_target_properties(lz4::lz4_shared PROPERTIES IMPORTED_LOCATION "${_SHARED_LIB}")
  target_include_directories(lz4::lz4_shared SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  add_dependencies(lz4::lz4_shared ${_EP_BASE})

  add_dependencies(am_airtree_dependencies lz4::lz4_shared)

  add_library(lz4::lz4_static STATIC IMPORTED GLOBAL)
  set_target_properties(lz4::lz4_static PROPERTIES IMPORTED_LOCATION "${_STATIC_LIB}")
  target_include_directories(lz4::lz4_static SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  add_dependencies(lz4::lz4_static ${_EP_BASE})

  add_dependencies(am_airtree_dependencies lz4::lz4_static)

endfunction()

configure_lz4()

if (AIRMETTLE_AIRTREE_USE_SHARED_LIBS)
  add_library(lz4::lz4 ALIAS lz4::lz4_shared)
else ()
  add_library(lz4::lz4 ALIAS lz4::lz4_static)
endif ()
