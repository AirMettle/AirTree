include_guard(GLOBAL)

include(GNUInstallDirs)
include(ExternalProject)

function(external_configure_curl _EP_BASE _EP_BUILD_DIR _INSTALL_DIR _STATIC_LIB _SHARED_LIB)
  get_property(_OPENSSL_INSTALL_DIR GLOBAL PROPERTY "AIRTREE_DEP_INSTALL_DIR_openssl")
  set(DOWNLOAD_OPTIONS
    GIT_REPOSITORY "https://github.com/curl/curl.git"
    GIT_TAG "${CURL_VERSION}"
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED FALSE)

  set(_BYPRODUCTS "")
  list(APPEND _BYPRODUCTS "${_SHARED_LIB}")
  list(APPEND _BYPRODUCTS "${_STATIC_LIB}")

  ExternalProject_Add(
    ${_EP_BASE}
    PREFIX ${_EP_BUILD_DIR}
    ${DOWNLOAD_OPTIONS}
    DEPENDS openssl_ep
    EXCLUDE_FROM_ALL ON
    INSTALL_DIR ${_INSTALL_DIR}
    CMAKE_ARGS
    -DCMAKE_INSTALL_LIBDIR=lib
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DCMAKE_BUILD_TYPE=${AIRMETTLE_AIRTREE_DEPS_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=${_INSTALL_DIR}
    -DBUILD_SHARED_LIBS=OFF
    -DCURL_USE_OPENSSL=ON
    -DOPENSSL_ROOT_DIR=${_OPENSSL_INSTALL_DIR}
    -DCURL_USE_LIBPSL=OFF
    -DCURL_BROTLI=OFF
    -DHAVE_BROTLI=OFF
    -DUSE_LIBPSL=OFF
    -DUSE_LIBIDN2=OFF
    INSTALL_BYPRODUCTS ${_BYPRODUCTS}
    BUILD_COMMAND cmake --build . --config ${AIRMETTLE_AIRTREE_DEPS_BUILD_TYPE} -- -j${NPROC}
    INSTALL_COMMAND cmake --build . --target install
  )
  file(MAKE_DIRECTORY "${_INSTALL_DIR}/include")
endfunction()

function(configure_curl)
  set(_DEPS_DIR "${AIRMETTLE_AIRTREE_DEPENDENCY_DIR}")
  set(_EP_BASE "curl_ep")
  set(_EP_BUILD_DIR "${_DEPS_DIR}/${_EP_BASE}/build")

  airtree_dep_try_cache(DEP_NAME curl EP_DIR "${_DEPS_DIR}/${_EP_BASE}" CACHE_HIT _cache_hit INSTALL_DIR _INSTALL_DIR)

  set(_SHARED_LIB "${_INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}curl${CMAKE_SHARED_LIBRARY_SUFFIX}")
  set(_STATIC_LIB "${_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}curl${CMAKE_STATIC_LIBRARY_SUFFIX}")

  if(_cache_hit)
    message(STATUS "${_EP_BASE} restored from cache.")
    add_custom_target(${_EP_BASE})
  elseif(NOT (EXISTS "${_SHARED_LIB}" OR EXISTS "${_STATIC_LIB}"))
    message(STATUS "${_EP_BASE} not found at ${_INSTALL_DIR}. Will download and build it.")
    external_configure_curl(${_EP_BASE} ${_EP_BUILD_DIR} ${_INSTALL_DIR} ${_STATIC_LIB} ${_SHARED_LIB})
    add_custom_target(clean_curl_build ALL
      COMMAND ${CMAKE_COMMAND} -E remove_directory ${_EP_BUILD_DIR}
      COMMENT "Cleaning up curl build directory after installation"
    )
    add_dependencies(clean_curl_build ${_EP_BASE})
    airtree_dep_mark_built(EP_TARGET ${_EP_BASE} INSTALL_DIR "${_INSTALL_DIR}")
  else()
    message(STATUS "${_EP_BASE} found at ${_INSTALL_DIR}. Skipping download and build.")
    add_custom_target(${_EP_BASE}) # Dummy target to keep dependencies working
  endif()

  set(_DEPS "")
  list(APPEND _DEPS openssl::openssl openssl::crypto zlib::zlib lz4::lz4)

  add_library(curl::curl_shared SHARED IMPORTED GLOBAL)
  set_target_properties(curl::curl_shared PROPERTIES IMPORTED_LOCATION "${_SHARED_LIB}")
  target_include_directories(curl::curl_shared SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  target_link_libraries(curl::curl_shared INTERFACE ${_DEPS})
  add_dependencies(curl::curl_shared ${_EP_BASE})

  add_dependencies(am_airtree_dependencies curl::curl_shared)

  add_library(curl::curl_static STATIC IMPORTED GLOBAL)
  set_target_properties(curl::curl_static PROPERTIES IMPORTED_LOCATION "${_STATIC_LIB}")
  target_include_directories(curl::curl_static SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  target_link_libraries(curl::curl_static INTERFACE ${_DEPS})
  add_dependencies(curl::curl_static ${_EP_BASE})

  add_dependencies(am_airtree_dependencies curl::curl_static)

endfunction()

configure_curl()

if (AIRMETTLE_AIRTREE_USE_SHARED_LIBS)
  add_library(curl::curl ALIAS curl::curl_shared)
else ()
  add_library(curl::curl ALIAS curl::curl_static)
endif ()
