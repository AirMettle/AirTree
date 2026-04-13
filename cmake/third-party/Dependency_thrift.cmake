include_guard(GLOBAL)

include(GNUInstallDirs)
include(ExternalProject)

function(external_configure_thrift _EP_BASE _EP_BUILD_DIR _INSTALL_DIR _THRIFT_SHARED_LIB _THRIFT_STATIC_LIB)
  get_property(_OPENSSL_INSTALL_DIR GLOBAL PROPERTY "AIRTREE_DEP_INSTALL_DIR_openssl")
  get_property(_BOOST_INSTALL_DIR GLOBAL PROPERTY "AIRTREE_DEP_INSTALL_DIR_boost")
  set(DOWNLOAD_OPTIONS
    GIT_REPOSITORY "https://github.com/apache/thrift.git"
    GIT_TAG "${THRIFT_VERSION}"
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED FALSE)

  set(_BYPRODUCTS "")
  list(APPEND _BYPRODUCTS "${_THRIFT_SHARED_LIB}")
  list(APPEND _BYPRODUCTS "${_THRIFT_STATIC_LIB}")

  # find_package(ZLIB REQUIRED)

  ExternalProject_Add(
    ${_EP_BASE}
    PREFIX ${_EP_BUILD_DIR}
    ${DOWNLOAD_OPTIONS}
    EXCLUDE_FROM_ALL ON
    DEPENDS openssl_ep boost_ep zlib_ep
    INSTALL_DIR ${_INSTALL_DIR}
    GIT_SUBMODULES_RECURSE 1
    SOURCE_DIR ${_EP_BUILD_DIR}/build/src/${_EP_BASE}
    # SOURCE_SUBDIR "lib/cpp"
    CMAKE_ARGS
    -DOPENSSL_ROOT_DIR=${_OPENSSL_INSTALL_DIR}
    -DOPENSSL_INCLUDE_DIR=${_OPENSSL_INSTALL_DIR}/include
    -DBoost_INCLUDE_DIR=${_BOOST_INSTALL_DIR}/include
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DCMAKE_BUILD_TYPE=${AIRMETTLE_AIRTREE_DEPS_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=${_INSTALL_DIR}
    -DBUILD_SHARED_LIBS=OFF
    -DWITH_ZLIB=ON
    -DWITH_OPENSSL=OFF
    -DWITH_LIBEVENT=OFF
    -DBUILD_TESTING=OFF
    -DWITH_QT5=OFF
    -DWITH_JAVA=OFF
    -DWITH_JAVASCRIPT=OFF
    -DWITH_NODEJS=OFF
    -DWITH_PYTHON=OFF
    INSTALL_BYPRODUCTS ${_BYPRODUCTS}
    BUILD_COMMAND cmake --build . --config ${AIRMETTLE_AIRTREE_DEPS_BUILD_TYPE} -- -j${NPROC}
    INSTALL_COMMAND cmake --build . --target install
  )

  file(MAKE_DIRECTORY "${_INSTALL_DIR}/include")
endfunction()

function(configure_thrift)
  set(_DEPS_DIR "${AIRMETTLE_AIRTREE_DEPENDENCY_DIR}")
  set(_EP_BASE "thrift_ep")
  set(_EP_BUILD_DIR "${_DEPS_DIR}/${_EP_BASE}/build")

  airtree_dep_try_cache(DEP_NAME thrift EP_DIR "${_DEPS_DIR}/${_EP_BASE}" CACHE_HIT _cache_hit INSTALL_DIR _INSTALL_DIR)

  set(_THRIFT_SHARED_LIB "${_INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}thrift${CMAKE_SHARED_LIBRARY_SUFFIX}")
  set(_THRIFT_STATIC_LIB "${_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}thrift${CMAKE_STATIC_LIBRARY_SUFFIX}")

  if(_cache_hit)
    message(STATUS "${_EP_BASE} restored from cache.")
    add_custom_target(${_EP_BASE})
  elseif(NOT (EXISTS "${_THRIFT_SHARED_LIB}" OR EXISTS "${_THRIFT_STATIC_LIB}"))
    message(STATUS "${_EP_BASE} not found at ${_INSTALL_DIR}. Will download and build it.")
    external_configure_thrift(${_EP_BASE} ${_EP_BUILD_DIR} ${_INSTALL_DIR} ${_THRIFT_SHARED_LIB} ${_THRIFT_STATIC_LIB})
    add_custom_target(clean_thrift_build ALL
      COMMAND ${CMAKE_COMMAND} -E remove_directory ${_EP_BUILD_DIR}
      COMMENT "Cleaning up thrift_build directory after installation"
    )
    add_dependencies(clean_thrift_build ${_EP_BASE})
    airtree_dep_mark_built(EP_TARGET ${_EP_BASE} INSTALL_DIR "${_INSTALL_DIR}")
  else()
    message(STATUS "${_EP_BASE} found at ${_INSTALL_DIR}. Skipping download and build.")
    add_custom_target(${_EP_BASE}) # Dummy target to keep dependencies working
  endif()

  set(_DEPS "")
  list(APPEND _DEPS zlib::zlib boost::system openssl::openssl openssl::crypto)

  add_library(thrift::thrift_shared SHARED IMPORTED GLOBAL)
  set_target_properties(thrift::thrift_shared PROPERTIES IMPORTED_LOCATION "${_THRIFT_SHARED_LIB}")
  target_include_directories(thrift::thrift_shared SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  target_link_libraries(thrift::thrift_shared INTERFACE ${_DEPS})
  add_dependencies(thrift::thrift_shared ${_EP_BASE})

  add_dependencies(am_airtree_dependencies thrift::thrift_shared)

  add_library(thrift::thrift_static STATIC IMPORTED GLOBAL)
  set_target_properties(thrift::thrift_static PROPERTIES IMPORTED_LOCATION "${_THRIFT_STATIC_LIB}")
  target_include_directories(thrift::thrift_static SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  target_link_libraries(thrift::thrift_static INTERFACE ${_DEPS})
  add_dependencies(thrift::thrift_static ${_EP_BASE})

  add_dependencies(am_airtree_dependencies thrift::thrift_static)

endfunction()

configure_thrift()

if (AIRMETTLE_AIRTREE_USE_SHARED_LIBS)
  add_library(thrift::thrift ALIAS thrift::thrift_shared)
else ()
  add_library(thrift::thrift ALIAS thrift::thrift_static)
endif ()
