include_guard(GLOBAL)

include(GNUInstallDirs)
include(ExternalProject)

function(external_configure_hdf5 _EP_BASE _EP_BUILD_DIR _INSTALL_DIR _SHARED_LIB _STATIC_LIB _SHARED_LIB_hl _STATIC_LIB_hl)
  get_property(_ZLIB_INSTALL_DIR GLOBAL PROPERTY "AIRTREE_DEP_INSTALL_DIR_zlib")
  # Remote
  set(DOWNLOAD_OPTIONS
    GIT_REPOSITORY "https://github.com/HDFGroup/hdf5.git"
    GIT_TAG "${HDF5_VERSION}"
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED FALSE)

  set(_BYPRODUCTS "")
  list(APPEND _BYPRODUCTS "${_SHARED_LIB}")
  list(APPEND _BYPRODUCTS "${_STATIC_LIB}")
  list(APPEND _BYPRODUCTS "${_SHARED_LIB_hl}")
  list(APPEND _BYPRODUCTS "${_STATIC_LIB_hl}")

  # Find ZLIB as it is required for HDF5 and NetCDF4
  # find_package(ZLIB REQUIRED)


  ExternalProject_Add(
    ${_EP_BASE}
    PREFIX ${_EP_BUILD_DIR}
    ${DOWNLOAD_OPTIONS}
    EXCLUDE_FROM_ALL ON
    DEPENDS zlib_ep
    INSTALL_DIR ${_INSTALL_DIR}
    CMAKE_ARGS
    -DCMAKE_INSTALL_LIBDIR=lib
    -DCMAKE_BUILD_TYPE=${AIRMETTLE_AIRTREE_DEPS_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=${_INSTALL_DIR}
    -DHDF5_ENABLE_Z_LIB_SUPPORT=True
    -DZLIB_LIBRARY=${_ZLIB_INSTALL_DIR}/lib/libz.a
    -DZLIB_INCLUDE_DIR=${_ZLIB_INSTALL_DIR}/include
    -DBUILD_SHARED_LIBS=OFF
    INSTALL_BYPRODUCTS ${_BYPRODUCTS}
    BUILD_COMMAND cmake --build . --config ${AIRMETTLE_AIRTREE_DEPS_BUILD_TYPE} -- -j${NPROC}
    INSTALL_COMMAND cmake --build . --target install
  )
  
  file(MAKE_DIRECTORY "${_INSTALL_DIR}/include")
endfunction()

function(configure_hdf5)

  set(_DEPS_DIR "${AIRMETTLE_AIRTREE_DEPENDENCY_DIR}")
  set(_EP_BASE "hdf5_ep")
  set(_EP_BUILD_DIR "${_DEPS_DIR}/${_EP_BASE}/build")

  airtree_dep_try_cache(DEP_NAME hdf5 EP_DIR "${_DEPS_DIR}/${_EP_BASE}" CACHE_HIT _cache_hit INSTALL_DIR _INSTALL_DIR)

  # Define expected install outputs
  set(_SHARED_LIB "${_INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}hdf5${CMAKE_SHARED_LIBRARY_SUFFIX}")
  set(_STATIC_LIB "${_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}hdf5${CMAKE_STATIC_LIBRARY_SUFFIX}")
  set(_SHARED_LIB_hl "${_INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}hdf5_hl${CMAKE_SHARED_LIBRARY_SUFFIX}")
  set(_STATIC_LIB_hl "${_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}hdf5_hl${CMAKE_STATIC_LIBRARY_SUFFIX}")
  # set(_SHARED_LIB_tools "${_INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}hdf5_tools${CMAKE_SHARED_LIBRARY_SUFFIX}")
  # set(_STATIC_LIB_tools "${_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}hdf5_tools${CMAKE_STATIC_LIBRARY_SUFFIX}")

  if(_cache_hit)
    message(STATUS "${_EP_BASE} restored from cache.")
    add_custom_target(${_EP_BASE})
  elseif(NOT (EXISTS "${_SHARED_LIB}" OR EXISTS "${_STATIC_LIB}"))
    message(STATUS "${_EP_BASE} not found at ${_INSTALL_DIR}. Will download and build it.")
    external_configure_hdf5(${_EP_BASE} ${_EP_BUILD_DIR} ${_INSTALL_DIR} ${_SHARED_LIB} ${_STATIC_LIB} ${_SHARED_LIB_hl} ${_STATIC_LIB_hl})
    add_custom_target(clean_hdf5_build ALL
      COMMAND ${CMAKE_COMMAND} -E remove_directory ${_EP_BUILD_DIR}
      COMMENT "Cleaning up hdf5_build directory after installation"
    )
    add_dependencies(clean_hdf5_build ${_EP_BASE})
    airtree_dep_mark_built(EP_TARGET ${_EP_BASE} INSTALL_DIR "${_INSTALL_DIR}")
  else()
    message(STATUS "${_EP_BASE} found at ${_INSTALL_DIR}. Skipping download and build.")
    add_custom_target(${_EP_BASE}) # Create a dummy target to ensure dependencies are met
  endif()

  # IMPORTED TARGET: hdf5::hdf5_shared
  add_library(hdf5::hdf5_shared SHARED IMPORTED GLOBAL)
  set_target_properties(hdf5::hdf5_shared PROPERTIES IMPORTED_LOCATION "${_SHARED_LIB}")
  target_include_directories(hdf5::hdf5_shared SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  add_dependencies(hdf5::hdf5_shared ${_EP_BASE})

  add_dependencies(am_airtree_dependencies hdf5::hdf5_shared)

  # IMPORTED TARGET: hdf5::hdf5_static
  add_library(hdf5::hdf5_static STATIC IMPORTED GLOBAL)
  set_target_properties(hdf5::hdf5_static PROPERTIES IMPORTED_LOCATION "${_STATIC_LIB}")
  target_include_directories(hdf5::hdf5_static SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  add_dependencies(hdf5::hdf5_static ${_EP_BASE})

  add_dependencies(am_airtree_dependencies hdf5::hdf5_static)

  # IMPORTED TARGET: hdf5::hdf5_hl_shared
  add_library(hdf5::hdf5_hl_shared SHARED IMPORTED GLOBAL)
  set_target_properties(hdf5::hdf5_hl_shared PROPERTIES IMPORTED_LOCATION "${_SHARED_LIB_hl}")
  target_include_directories(hdf5::hdf5_hl_shared SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  add_dependencies(hdf5::hdf5_hl_shared ${_EP_BASE})

  add_dependencies(am_airtree_dependencies hdf5::hdf5_hl_shared)

  # IMPORTED TARGET: hdf5::hdf5_hl_static
  add_library(hdf5::hdf5_hl_static STATIC IMPORTED GLOBAL)
  set_target_properties(hdf5::hdf5_hl_static PROPERTIES IMPORTED_LOCATION "${_STATIC_LIB_hl}")
  target_include_directories(hdf5::hdf5_hl_static SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  add_dependencies(hdf5::hdf5_hl_static ${_EP_BASE})

  add_dependencies(am_airtree_dependencies hdf5::hdf5_hl_static)

  # # IMPORTED TARGET: hdf5::hdf5_tools_shared
  # add_library(hdf5::hdf5_tools_shared SHARED IMPORTED GLOBAL)
  # set_target_properties(hdf5::hdf5_tools_shared PROPERTIES IMPORTED_LOCATION "${_SHARED_LIB_tools}")
  # target_include_directories(hdf5::hdf5_tools_shared SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  # add_dependencies(hdf5::hdf5_tools_shared ${_EP_BASE})

  # add_dependencies(am_airtree_dependencies hdf5::hdf5_tools_shared)

  # # IMPORTED TARGET: hdf5::hdf5_tools_static
  # add_library(hdf5::hdf5_tools_static STATIC IMPORTED GLOBAL)
  # set_target_properties(hdf5::hdf5_tools_static PROPERTIES IMPORTED_LOCATION "${_STATIC_LIB_tools}")
  # target_include_directories(hdf5::hdf5_tools_static SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  # add_dependencies(hdf5::hdf5_tools_static ${_EP_BASE})

  # add_dependencies(am_airtree_dependencies hdf5::hdf5_tools_static)

endfunction()

configure_hdf5()

# Alias hdf5::hdf5 to the shared or static lib we intend to use
if (AIRMETTLE_AIRTREE_USE_SHARED_LIBS)
  add_library(hdf5::hdf5 ALIAS hdf5::hdf5_shared)
  add_library(hdf5::hdf5_hl ALIAS hdf5::hdf5_hl_shared)
  # add_library(hdf5::hdf5_tools ALIAS hdf5::hdf5_tools_shared)
else ()
  add_library(hdf5::hdf5 ALIAS hdf5::hdf5_static)
  add_library(hdf5::hdf5_hl ALIAS hdf5::hdf5_hl_static)
  # add_library(hdf5::hdf5_tools ALIAS hdf5::hdf5_tools_static)
endif ()
