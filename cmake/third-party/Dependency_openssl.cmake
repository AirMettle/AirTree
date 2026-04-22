include_guard(GLOBAL)

include(GNUInstallDirs)
include(ExternalProject)

function(external_configure_openssl _EP_BASE _EP_BUILD_DIR _INSTALL_DIR _STATIC_LIB _SHARED_LIB _STATIC_CRYPTO_LIB _SHARED_CRYPTO_LIB)
  set(DOWNLOAD_OPTIONS
    GIT_REPOSITORY "https://github.com/openssl/openssl.git"
    GIT_TAG "${OPENSSL_VERSION}"
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED FALSE)

  set(_BYPRODUCTS "")
  list(APPEND _BYPRODUCTS "${_SHARED_LIB}")
  list(APPEND _BYPRODUCTS "${_STATIC_LIB}")
  list(APPEND _BYPRODUCTS "${_SHARED_CRYPTO_LIB}")
  list(APPEND _BYPRODUCTS "${_STATIC_CRYPTO_LIB}")

  # OpenSSL's Configure uses --debug for debug builds
  if(AIRMETTLE_AIRTREE_DEPS_BUILD_TYPE STREQUAL "Debug")
    set(_OPENSSL_DEBUG_FLAG "--debug")
  else()
    set(_OPENSSL_DEBUG_FLAG "")
  endif()

  ExternalProject_Add(
    ${_EP_BASE}
    PREFIX ${_EP_BUILD_DIR}
    ${DOWNLOAD_OPTIONS}
    EXCLUDE_FROM_ALL ON
    INSTALL_DIR ${_INSTALL_DIR}
    CONFIGURE_COMMAND ./Configure ${_OPENSSL_DEBUG_FLAG} --prefix=${_INSTALL_DIR} --openssldir=${_INSTALL_DIR}/ssl --libdir=lib no-tests
    CMAKE_ARGS
    -DCMAKE_INSTALL_LIBDIR=lib
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    INSTALL_BYPRODUCTS ${_BYPRODUCTS}
    BUILD_COMMAND make -j${NPROC}
    INSTALL_COMMAND make install_sw
    BUILD_IN_SOURCE 1
    BUILD_ENV OPENSSL_LIBDIR=lib
  )

  file(MAKE_DIRECTORY "${_INSTALL_DIR}/include")
endfunction()

function(configure_openssl)
  set(_DEPS_DIR "${AIRMETTLE_AIRTREE_DEPENDENCY_DIR}")
  set(_EP_BASE "openssl_ep")
  set(_EP_BUILD_DIR "${_DEPS_DIR}/${_EP_BASE}/build")

  airtree_dep_try_cache(DEP_NAME openssl EP_DIR "${_DEPS_DIR}/${_EP_BASE}" CACHE_HIT _cache_hit INSTALL_DIR _INSTALL_DIR)

  set(_SHARED_LIB "${_INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}ssl${CMAKE_SHARED_LIBRARY_SUFFIX}")
  set(_STATIC_LIB "${_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ssl${CMAKE_STATIC_LIBRARY_SUFFIX}")
  set(_SHARED_CRYPTO_LIB "${_INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}crypto${CMAKE_SHARED_LIBRARY_SUFFIX}")
  set(_STATIC_CRYPTO_LIB "${_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}crypto${CMAKE_STATIC_LIBRARY_SUFFIX}")

  if(_cache_hit)
    message(STATUS "${_EP_BASE} restored from cache.")
    add_custom_target(${_EP_BASE})
  elseif(NOT (EXISTS "${_SHARED_LIB}" OR EXISTS "${_STATIC_LIB}"))
    message(STATUS "${_EP_BASE} not found at ${_INSTALL_DIR}. Will download and build it.")
    external_configure_openssl(${_EP_BASE} ${_EP_BUILD_DIR} ${_INSTALL_DIR} ${_STATIC_LIB} ${_SHARED_LIB} ${_STATIC_CRYPTO_LIB} ${_SHARED_CRYPTO_LIB})
    add_custom_target(clean_openssl_build ALL
      COMMAND ${CMAKE_COMMAND} -E remove_directory ${_EP_BUILD_DIR}
      COMMENT "Cleaning up openssl build directory after installation"
    )
    add_dependencies(clean_openssl_build ${_EP_BASE})
    airtree_dep_mark_built(EP_TARGET ${_EP_BASE} INSTALL_DIR "${_INSTALL_DIR}")
  else()
    message(STATUS "${_EP_BASE} found at ${_INSTALL_DIR}. Skipping download and build.")
    add_custom_target(${_EP_BASE}) # Dummy target to keep dependencies working
  endif()

  add_library(openssl::openssl_shared SHARED IMPORTED GLOBAL)
  set_target_properties(openssl::openssl_shared PROPERTIES IMPORTED_LOCATION "${_SHARED_LIB}")
  target_include_directories(openssl::openssl_shared SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  add_dependencies(openssl::openssl_shared ${_EP_BASE})

  add_dependencies(am_airtree_dependencies openssl::openssl_shared)

  add_library(openssl::openssl_static STATIC IMPORTED GLOBAL)
  set_target_properties(openssl::openssl_static PROPERTIES IMPORTED_LOCATION "${_STATIC_LIB}")
  target_include_directories(openssl::openssl_static SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  add_dependencies(openssl::openssl_static ${_EP_BASE})

  add_dependencies(am_airtree_dependencies openssl::openssl_static)

  add_library(openssl::openssl_crypto_shared SHARED IMPORTED GLOBAL)
  set_target_properties(openssl::openssl_crypto_shared PROPERTIES IMPORTED_LOCATION "${_SHARED_CRYPTO_LIB}")
  target_include_directories(openssl::openssl_crypto_shared SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  target_link_libraries(openssl::openssl_crypto_shared INTERFACE ${CMAKE_DL_LIBS})
  add_dependencies(openssl::openssl_crypto_shared ${_EP_BASE})

  add_dependencies(am_airtree_dependencies openssl::openssl_crypto_shared)

  add_library(openssl::openssl_crypto_static STATIC IMPORTED GLOBAL)
  set_target_properties(openssl::openssl_crypto_static PROPERTIES IMPORTED_LOCATION "${_STATIC_CRYPTO_LIB}")
  target_include_directories(openssl::openssl_crypto_static SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  target_link_libraries(openssl::openssl_crypto_static INTERFACE ${CMAKE_DL_LIBS})
  add_dependencies(openssl::openssl_crypto_static ${_EP_BASE})

  add_dependencies(am_airtree_dependencies openssl::openssl_crypto_static)


endfunction()

configure_openssl()

if (AIRMETTLE_AIRTREE_USE_SHARED_LIBS)
  add_library(openssl::openssl ALIAS openssl::openssl_shared)
  add_library(openssl::crypto ALIAS openssl::openssl_crypto_shared)
else ()
  add_library(openssl::openssl ALIAS openssl::openssl_static)
  add_library(openssl::crypto ALIAS openssl::openssl_crypto_static)
endif ()