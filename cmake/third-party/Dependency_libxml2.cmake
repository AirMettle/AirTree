include_guard(GLOBAL)

include(GNUInstallDirs)
include(ExternalProject)

function(external_configure_libxml2 _EP_BASE _EP_BUILD_DIR _INSTALL_DIR _SHARED_LIB _STATIC_LIB)
  set(DOWNLOAD_OPTIONS
    GIT_REPOSITORY "https://gitlab.gnome.org/GNOME/libxml2.git"
    GIT_TAG "${LIBXML2_VERSION}"
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
    -DCMAKE_INSTALL_LIBDIR=lib
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DCMAKE_BUILD_TYPE=${AIRMETTLE_AIRTREE_DEPS_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=${_INSTALL_DIR}
    -DBUILD_SHARED_LIBS=OFF
    -DLIBXML2_WITH_PYTHON=OFF
    -DLIBXML2_WITH_LZMA=OFF
    -DLIBXML2_WITH_ICONV=ON
    -DLIBXML2_WITH_ZLIB=OFF
    INSTALL_BYPRODUCTS ${_BYPRODUCTS}
    BUILD_COMMAND cmake --build . --config ${AIRMETTLE_AIRTREE_DEPS_BUILD_TYPE} -- -j${NPROC}
    INSTALL_COMMAND cmake --build . --target install
  )
  file(MAKE_DIRECTORY "${_INSTALL_DIR}/include")
endfunction()

function(configure_libxml2)
  set(_DEPS_DIR "${AIRMETTLE_AIRTREE_DEPENDENCY_DIR}")
  set(_EP_BASE "libxml2_ep")
  set(_EP_BUILD_DIR "${_DEPS_DIR}/${_EP_BASE}/build")

  airtree_dep_try_cache(DEP_NAME libxml2 EP_DIR "${_DEPS_DIR}/${_EP_BASE}" CACHE_HIT _cache_hit INSTALL_DIR _INSTALL_DIR)

  set(_SHARED_LIB "${_INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}xml2${CMAKE_SHARED_LIBRARY_SUFFIX}")
  set(_STATIC_LIB "${_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}xml2${CMAKE_STATIC_LIBRARY_SUFFIX}")

  if(_cache_hit)
    message(STATUS "${_EP_BASE} restored from cache.")
    add_custom_target(${_EP_BASE})
  elseif(NOT (EXISTS "${_SHARED_LIB}" OR EXISTS "${_STATIC_LIB}"))
    message(STATUS "${_EP_BASE} not found at ${_INSTALL_DIR}. Will download and build it.")
    external_configure_libxml2(${_EP_BASE} ${_EP_BUILD_DIR} ${_INSTALL_DIR} ${_SHARED_LIB} ${_STATIC_LIB})
    add_custom_target(clean_libxml2_build ALL
      COMMAND ${CMAKE_COMMAND} -E remove_directory ${_EP_BUILD_DIR}
      COMMENT "Cleaning up libxml2 build directory after installation"
    )
    add_dependencies(clean_libxml2_build ${_EP_BASE})
    airtree_dep_mark_built(EP_TARGET ${_EP_BASE} INSTALL_DIR "${_INSTALL_DIR}")
  else()
    message(STATUS "${_EP_BASE} found at ${_INSTALL_DIR}. Skipping download and build.")
    add_custom_target(${_EP_BASE})
  endif()

  set(_DEPS "")

  add_library(libxml2::libxml2_shared SHARED IMPORTED GLOBAL)
  set_target_properties(libxml2::libxml2_shared PROPERTIES IMPORTED_LOCATION "${_SHARED_LIB}")
  target_include_directories(libxml2::libxml2_shared SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  target_link_libraries(libxml2::libxml2_shared INTERFACE ${_DEPS})
  add_dependencies(libxml2::libxml2_shared ${_EP_BASE})
  add_dependencies(am_airtree_dependencies libxml2::libxml2_shared)

  add_library(libxml2::libxml2_static STATIC IMPORTED GLOBAL)
  set_target_properties(libxml2::libxml2_static PROPERTIES IMPORTED_LOCATION "${_STATIC_LIB}")
  target_include_directories(libxml2::libxml2_static SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  target_link_libraries(libxml2::libxml2_static INTERFACE ${_DEPS})
  add_dependencies(libxml2::libxml2_static ${_EP_BASE})
  add_dependencies(am_airtree_dependencies libxml2::libxml2_static)

endfunction()

configure_libxml2()

if (AIRMETTLE_AIRTREE_USE_SHARED_LIBS)
  add_library(libxml2::libxml2 ALIAS libxml2::libxml2_shared)
else ()
  add_library(libxml2::libxml2 ALIAS libxml2::libxml2_static)
endif ()
