include_guard(GLOBAL)

include(GNUInstallDirs)
include(ExternalProject)

function(external_configure_netcdf4 _EP_BASE _EP_BUILD_DIR _INSTALL_DIR _SHARED_LIB _STATIC_LIB)
  get_property(_ZLIB_INSTALL_DIR GLOBAL PROPERTY "AIRTREE_DEP_INSTALL_DIR_zlib")
  get_property(_HDF5_INSTALL_DIR GLOBAL PROPERTY "AIRTREE_DEP_INSTALL_DIR_hdf5")
  # Remote
  set(DOWNLOAD_OPTIONS
    GIT_REPOSITORY "https://github.com/Unidata/netcdf-c.git"
    GIT_TAG "${NETCDF4_VERSION}"
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED FALSE)
  
  set(_BYPRODUCTS "")
  list(APPEND _BYPRODUCTS "${_SHARED_LIB}")
  list(APPEND _BYPRODUCTS "${_STATIC_LIB}")


  if(WIN32)
    set(_ZLIB_STATIC_LIB "${_ZLIB_INSTALL_DIR}/lib/zlibstatic.lib")
  else()
    set(_ZLIB_STATIC_LIB "${_ZLIB_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}z${CMAKE_STATIC_LIBRARY_SUFFIX}")
  endif()

  ExternalProject_Add(
    ${_EP_BASE}
    DEPENDS hdf5_ep                         
    PREFIX ${_EP_BUILD_DIR}
    ${DOWNLOAD_OPTIONS}
    EXCLUDE_FROM_ALL ON
    DEPENDS curl_ep zlib_ep
    INSTALL_DIR ${_INSTALL_DIR}
    CMAKE_ARGS
    -DCMAKE_INSTALL_LIBDIR=lib
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DCMAKE_BUILD_TYPE=${AIRMETTLE_AIRTREE_DEPS_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=${_INSTALL_DIR}
    -DBUILD_SHARED_LIBS=OFF
    -DHDF5_ROOT=${_HDF5_INSTALL_DIR}
    -DHDF5_DIR=${_HDF5_INSTALL_DIR}/cmake/hdf5
    -DZLIB_LIBRARY=${_ZLIB_STATIC_LIB}
    -DZLIB_INCLUDE_DIR=${_ZLIB_INSTALL_DIR}/include
    -DENABLE_BYTERANGE=OFF
    -DENABLE_DAP=OFF
    -DCMAKE_PREFIX_PATH=${_HDF5_INSTALL_DIR}
    INSTALL_BYPRODUCTS ${_BYPRODUCTS}
    BUILD_COMMAND cmake --build . --config ${AIRMETTLE_AIRTREE_DEPS_BUILD_TYPE} -- -j${NPROC}
    INSTALL_COMMAND cmake --build . --target install
  )

  file(MAKE_DIRECTORY "${_INSTALL_DIR}/include")
endfunction()

function(configure_netcdf4)

  set(_DEPS_DIR "${AIRMETTLE_AIRTREE_DEPENDENCY_DIR}")
  set(_EP_BASE "netcdf4_ep")
  set(_EP_BUILD_DIR "${_DEPS_DIR}/${_EP_BASE}/build")

  airtree_dep_try_cache(DEP_NAME netcdf4 EP_DIR "${_DEPS_DIR}/${_EP_BASE}" CACHE_HIT _cache_hit INSTALL_DIR _INSTALL_DIR)

  # Define expected install outputs
  set(_SHARED_LIB "${_INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}netcdf${CMAKE_SHARED_LIBRARY_SUFFIX}")
  set(_STATIC_LIB "${_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}netcdf${CMAKE_STATIC_LIBRARY_SUFFIX}")

  if(_cache_hit)
    message(STATUS "${_EP_BASE} restored from cache.")
    add_custom_target(${_EP_BASE})
  elseif(NOT (EXISTS "${_SHARED_LIB}" OR EXISTS "${_STATIC_LIB}"))
    message(STATUS "${_EP_BASE} not found at ${_INSTALL_DIR}. Will download and build it.")
    external_configure_netcdf4(${_EP_BASE} ${_EP_BUILD_DIR} ${_INSTALL_DIR} ${_SHARED_LIB} ${_STATIC_LIB})
    add_custom_target(clean_netcdf_build ALL
      COMMAND ${CMAKE_COMMAND} -E remove_directory ${_EP_BUILD_DIR}
      COMMENT "Cleaning up netcdf_build directory after installation"
    )
    add_dependencies(clean_netcdf_build ${_EP_BASE})
    airtree_dep_mark_built(EP_TARGET ${_EP_BASE} INSTALL_DIR "${_INSTALL_DIR}")
  else()
    message(STATUS "${_EP_BASE} found at ${_INSTALL_DIR}. Skipping download and build.")
    add_custom_target(${_EP_BASE}) # Create a dummy target to ensure dependencies are met
  endif()

  # set(_DEPS hdf5::hdf5_hl hdf5::hdf5 hdf5::hdf5_tools curl::curl libxml2::libxml2)
  set(_DEPS hdf5::hdf5_hl hdf5::hdf5 curl::curl libxml2::libxml2)

  # IMPORTED TARGET: netcdf::netcdf_shared
  add_library(netcdf::netcdf_shared SHARED IMPORTED GLOBAL)
  set_target_properties(netcdf::netcdf_shared PROPERTIES IMPORTED_LOCATION "${_SHARED_LIB}")
  target_include_directories(netcdf::netcdf_shared SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  target_link_libraries(netcdf::netcdf_shared INTERFACE ${_DEPS})
  add_dependencies(netcdf::netcdf_shared ${_EP_BASE})

  add_dependencies(am_airtree_dependencies netcdf::netcdf_shared)

  # IMPORTED TARGET: netcdf::netcdf_static
  add_library(netcdf::netcdf_static STATIC IMPORTED GLOBAL)
  set_target_properties(netcdf::netcdf_static PROPERTIES IMPORTED_LOCATION "${_STATIC_LIB}")
  target_include_directories(netcdf::netcdf_static SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  target_link_libraries(netcdf::netcdf_static INTERFACE ${_DEPS})
  add_dependencies(netcdf::netcdf_static ${_EP_BASE})

  add_dependencies(am_airtree_dependencies netcdf::netcdf_static)

endfunction()

configure_netcdf4()

# Alias netcdf::netcdf to the shared or static lib we intend to use
if (AIRMETTLE_AIRTREE_USE_SHARED_LIBS)
  add_library(netcdf::netcdf ALIAS netcdf::netcdf_shared)
else ()
  add_library(netcdf::netcdf ALIAS netcdf::netcdf_static)
endif ()
