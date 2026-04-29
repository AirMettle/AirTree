include_guard(GLOBAL)

include(GNUInstallDirs)
include(ExternalProject)



# Build and install Boost libraries (system, filesystem, program_options)
function(external_configure_boost _EP_BASE _EP_BUILD_DIR _INSTALL_DIR _BOOST_SYSTEM_SHARED _BOOST_SYSTEM_STATIC _BOOST_FILESYSTEM_SHARED _BOOST_FILESYSTEM_STATIC _BOOST_PROGRAM_OPTIONS_SHARED _BOOST_PROGRAM_OPTIONS_STATIC)
  set(DOWNLOAD_OPTIONS
    GIT_REPOSITORY "https://github.com/boostorg/boost.git"
    GIT_TAG "${BOOST_VERSION}"
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED FALSE)

  if(WIN32)
    file(WRITE "${_EP_BUILD_DIR}/user-config.jam" "using msvc : 14.3 : cl.exe : <setup>\"\" ;\n")
    set(_BOOTSTRAP_CMD bootstrap.bat msvc)
    set(_B2_EXE b2.exe)
    set(_B2_OS_ARGS toolset=msvc-14.3 address-model=64 architecture=x86 --user-config=${_EP_BUILD_DIR}/user-config.jam)
  else()
    set(_BOOTSTRAP_CMD ./bootstrap.sh --prefix=${_INSTALL_DIR})
    set(_B2_EXE ./b2)
    set(_B2_OS_ARGS "")
  endif()

  if (AIRTREE_ENABLE_BENCHMARKS OR AIRTREE_ENABLE_TESTING)
      if(UNIX)
          list(APPEND _BOOTSTRAP_CMD --with-libraries=system,filesystem,program_options)
      endif()

      set(BOOST_CONFIGURE_CMD ${_BOOTSTRAP_CMD})
      set(BOOST_BUILD_CMD ${_B2_EXE} headers)
      set(BOOST_INSTALL_CMD ${_B2_EXE} install -j${NPROC} --prefix=${_INSTALL_DIR} --with-system --with-filesystem --with-program_options --layout=system variant=${AIRMETTLE_AIRTREE_BOOST_VARIANT} link=static,shared threading=multi runtime-link=shared,static ${_B2_OS_ARGS})
      
      set(_BYPRODUCTS "")
      list(APPEND _BYPRODUCTS "${_BOOST_SYSTEM_SHARED}")
      list(APPEND _BYPRODUCTS "${_BOOST_SYSTEM_STATIC}")
      list(APPEND _BYPRODUCTS "${_BOOST_FILESYSTEM_SHARED}")
      list(APPEND _BYPRODUCTS "${_BOOST_FILESYSTEM_STATIC}")
      list(APPEND _BYPRODUCTS "${_BOOST_PROGRAM_OPTIONS_SHARED}")
      list(APPEND _BYPRODUCTS "${_BOOST_PROGRAM_OPTIONS_STATIC}")
      
  else()
      message(STATUS "[Boost] Lean mode detected: Extracting Boost headers only.")
      
      set(BOOST_CONFIGURE_CMD ${_BOOTSTRAP_CMD})
      set(BOOST_BUILD_CMD ${_B2_EXE} headers)
      
      set(BOOST_INSTALL_CMD ${CMAKE_COMMAND} -E copy_directory "${_EP_BUILD_DIR}/src/${_EP_BASE}/boost" "${_INSTALL_DIR}/include/boost")
      
      set(_BYPRODUCTS "")
  endif()

  ExternalProject_Add(
    ${_EP_BASE}
    PREFIX ${_EP_BUILD_DIR}
    ${DOWNLOAD_OPTIONS}
    EXCLUDE_FROM_ALL ON
    INSTALL_DIR ${_INSTALL_DIR}
    GIT_SUBMODULES_RECURSE 1
    CONFIGURE_COMMAND ${BOOST_CONFIGURE_CMD}
    INSTALL_BYPRODUCTS ${_BYPRODUCTS}
    BUILD_COMMAND ${BOOST_BUILD_CMD}
    INSTALL_COMMAND ${BOOST_INSTALL_CMD}
    BUILD_IN_SOURCE 1
  )
  file(MAKE_DIRECTORY "${_INSTALL_DIR}/include")
endfunction()



function(configure_boost)
  set(_DEPS_DIR "${AIRMETTLE_AIRTREE_DEPENDENCY_DIR}")
  set(_EP_BASE "boost_ep")
  set(_EP_BUILD_DIR "${_DEPS_DIR}/${_EP_BASE}/build")

  airtree_dep_try_cache(DEP_NAME boost EP_DIR "${_DEPS_DIR}/${_EP_BASE}" CACHE_HIT _cache_hit INSTALL_DIR _INSTALL_DIR)

  set(_BOOST_SYSTEM_SHARED "${_INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}boost_system${CMAKE_SHARED_LIBRARY_SUFFIX}")
  set(_BOOST_SYSTEM_STATIC "${_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}boost_system${CMAKE_STATIC_LIBRARY_SUFFIX}")
  set(_BOOST_FILESYSTEM_SHARED "${_INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}boost_filesystem${CMAKE_SHARED_LIBRARY_SUFFIX}")
  set(_BOOST_FILESYSTEM_STATIC "${_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}boost_filesystem${CMAKE_STATIC_LIBRARY_SUFFIX}")
  set(_BOOST_PROGRAM_OPTIONS_SHARED "${_INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}boost_program_options${CMAKE_SHARED_LIBRARY_SUFFIX}")
  set(_BOOST_PROGRAM_OPTIONS_STATIC "${_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}boost_program_options${CMAKE_STATIC_LIBRARY_SUFFIX}")

  file(GLOB BOOST_UUID_CONTENTS "${_INSTALL_DIR}/include/*")

  if(_cache_hit)
    message(STATUS "${_EP_BASE} restored from cache.")
    add_custom_target(${_EP_BASE})
  elseif(NOT (EXISTS "${_BOOST_SYSTEM_SHARED}" OR EXISTS "${_BOOST_SYSTEM_STATIC}") OR
     NOT (EXISTS "${_BOOST_FILESYSTEM_SHARED}" OR EXISTS "${_BOOST_FILESYSTEM_STATIC}") OR
     NOT (EXISTS "${_BOOST_PROGRAM_OPTIONS_SHARED}" OR EXISTS "${_BOOST_PROGRAM_OPTIONS_STATIC}") OR
     NOT BOOST_UUID_CONTENTS)
    message(STATUS "${_EP_BASE} not found at ${_INSTALL_DIR}. Will download and build Boost.")
    external_configure_boost(${_EP_BASE} ${_EP_BUILD_DIR} ${_INSTALL_DIR}
      ${_BOOST_SYSTEM_SHARED} ${_BOOST_SYSTEM_STATIC}
      ${_BOOST_FILESYSTEM_SHARED} ${_BOOST_FILESYSTEM_STATIC}
      ${_BOOST_PROGRAM_OPTIONS_SHARED} ${_BOOST_PROGRAM_OPTIONS_STATIC})
    add_custom_target(clean_boost_build ALL
      COMMAND ${CMAKE_COMMAND} -E remove_directory ${_EP_BUILD_DIR}
      COMMENT "Cleaning up boost build directory after installation"
    )
    add_dependencies(clean_boost_build ${_EP_BASE})
    airtree_dep_mark_built(EP_TARGET ${_EP_BASE} INSTALL_DIR "${_INSTALL_DIR}")
  else()
    message(STATUS "${_EP_BASE} found at ${_INSTALL_DIR}. Skipping download and build.")
    add_custom_target(${_EP_BASE}) # Dummy target to keep dependencies working
  endif()

  # Imported targets for Boost::system
  add_library(boost::system_shared SHARED IMPORTED GLOBAL)
  set_target_properties(boost::system_shared PROPERTIES IMPORTED_LOCATION "${_BOOST_SYSTEM_SHARED}")
  target_include_directories(boost::system_shared SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  add_dependencies(boost::system_shared ${_EP_BASE})

  add_dependencies(am_airtree_dependencies boost::system_shared)

  add_library(boost::system_static STATIC IMPORTED GLOBAL)
  set_target_properties(boost::system_static PROPERTIES IMPORTED_LOCATION "${_BOOST_SYSTEM_STATIC}")
  target_include_directories(boost::system_static SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  add_dependencies(boost::system_static ${_EP_BASE})

  add_dependencies(am_airtree_dependencies boost::system_static)

  # Imported targets for Boost::filesystem
  add_library(boost::filesystem_shared SHARED IMPORTED GLOBAL)
  set_target_properties(boost::filesystem_shared PROPERTIES IMPORTED_LOCATION "${_BOOST_FILESYSTEM_SHARED}")
  target_include_directories(boost::filesystem_shared SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  add_dependencies(boost::filesystem_shared ${_EP_BASE})

  add_dependencies(am_airtree_dependencies boost::filesystem_shared)

  add_library(boost::filesystem_static STATIC IMPORTED GLOBAL)
  set_target_properties(boost::filesystem_static PROPERTIES IMPORTED_LOCATION "${_BOOST_FILESYSTEM_STATIC}")
  target_include_directories(boost::filesystem_static SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  add_dependencies(boost::filesystem_static ${_EP_BASE})

  add_dependencies(am_airtree_dependencies boost::filesystem_static)

  # Imported targets for Boost::program_options
  add_library(boost::program_options_shared SHARED IMPORTED GLOBAL)
  set_target_properties(boost::program_options_shared PROPERTIES IMPORTED_LOCATION "${_BOOST_PROGRAM_OPTIONS_SHARED}")
  target_include_directories(boost::program_options_shared SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  add_dependencies(boost::program_options_shared ${_EP_BASE})

  add_dependencies(am_airtree_dependencies boost::program_options_shared)

  add_library(boost::program_options_static STATIC IMPORTED GLOBAL)
  set_target_properties(boost::program_options_static PROPERTIES IMPORTED_LOCATION "${_BOOST_PROGRAM_OPTIONS_STATIC}")
  target_include_directories(boost::program_options_static SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  add_dependencies(boost::program_options_static ${_EP_BASE})

  add_dependencies(am_airtree_dependencies boost::program_options_static)

  # IMPORTED TARGET: boost_headers~
  add_library(boost_headers INTERFACE IMPORTED GLOBAL)
  target_include_directories(boost_headers SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  target_link_libraries(boost_headers INTERFACE ${_DEPS})
  add_dependencies(boost_headers ${_EP_BASE})

  add_dependencies(am_airtree_dependencies boost_headers)

endfunction()


configure_boost()

if (AIRMETTLE_AIRTREE_USE_SHARED_LIBS)
  add_library(boost::system ALIAS boost::system_shared)
  add_library(boost::filesystem ALIAS boost::filesystem_shared)
  add_library(boost::program_options ALIAS boost::program_options_shared)
else ()
  add_library(boost::system ALIAS boost::system_static)
  add_library(boost::filesystem ALIAS boost::filesystem_static)
  add_library(boost::program_options ALIAS boost::program_options_static)
endif ()
