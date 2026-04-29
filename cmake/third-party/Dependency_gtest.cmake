include_guard(GLOBAL)

include(GNUInstallDirs)
include(ExternalProject)

function(external_configure_gtest _EP_BASE _EP_BUILD_DIR _INSTALL_DIR _STATIC_LIB _SHARED_LIB _STATIC_MAIN_LIB _SHARED_MAIN_LIB)
  # Remote
  set(DOWNLOAD_OPTIONS
    GIT_REPOSITORY "https://github.com/google/googletest.git"
    GIT_TAG "${GTEST_VERSION}"
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED FALSE)

  set(_BYPRODUCTS "")
  list(APPEND _BYPRODUCTS "${_SHARED_LIB}")
  list(APPEND _BYPRODUCTS "${_STATIC_LIB}")
  list(APPEND _BYPRODUCTS "${_SHARED_MAIN_LIB}")
  list(APPEND _BYPRODUCTS "${_STATIC_MAIN_LIB}")
  
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
    -Dgtest_force_shared_crt=ON
    INSTALL_BYPRODUCTS ${_BYPRODUCTS}
    BUILD_COMMAND cmake --build . --config ${AIRMETTLE_AIRTREE_DEPS_BUILD_TYPE} -- -j${NPROC}
    INSTALL_COMMAND cmake --build . --target install
  )
  
  file(MAKE_DIRECTORY "${_INSTALL_DIR}/include")
endfunction()

function(configure_gtest)

  set(_DEPS_DIR "${AIRMETTLE_AIRTREE_DEPENDENCY_DIR}")
  set(_EP_BASE "gtest_ep")
  set(_EP_BUILD_DIR "${_DEPS_DIR}/${_EP_BASE}/build")

  airtree_dep_try_cache(DEP_NAME gtest EP_DIR "${_DEPS_DIR}/${_EP_BASE}" CACHE_HIT _cache_hit INSTALL_DIR _INSTALL_DIR)

  # Define expected install outputs
  set(_SHARED_LIB "${_INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}gtest${CMAKE_SHARED_LIBRARY_SUFFIX}")
  set(_STATIC_LIB "${_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}")
  set(_SHARED_MAIN_LIB "${_INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}gtest_main${CMAKE_SHARED_LIBRARY_SUFFIX}")
  set(_STATIC_MAIN_LIB "${_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX}")

  if(_cache_hit)
    message(STATUS "${_EP_BASE} restored from cache.")
    add_custom_target(${_EP_BASE})
  elseif(NOT (EXISTS "${_SHARED_LIB}" OR EXISTS "${_STATIC_LIB}"))
    message(STATUS "${_EP_BASE} not found at ${_INSTALL_DIR}. Will download and build it.")
    external_configure_gtest(${_EP_BASE} ${_EP_BUILD_DIR} ${_INSTALL_DIR} ${_STATIC_LIB} ${_SHARED_LIB} ${_STATIC_MAIN_LIB} ${_SHARED_MAIN_LIB})
    add_custom_target(clean_gtest_build ALL
      COMMAND ${CMAKE_COMMAND} -E remove_directory ${_EP_BUILD_DIR}
      COMMENT "Cleaning up gtest_build directory after installation"
    )
    add_dependencies(clean_gtest_build ${_EP_BASE})
    airtree_dep_mark_built(EP_TARGET ${_EP_BASE} INSTALL_DIR "${_INSTALL_DIR}")
  else()
    message(STATUS "${_EP_BASE} found at ${_INSTALL_DIR}. Skipping download and build.")
    add_custom_target(${_EP_BASE}) # Dummy target to keep dependencies working
  endif()

  # IMPORTED TARGET: gtest::gtest_shared
  add_library(gtest::gtest_shared SHARED IMPORTED GLOBAL)
  set_target_properties(gtest::gtest_shared PROPERTIES IMPORTED_LOCATION "${_SHARED_LIB}")
  target_include_directories(gtest::gtest_shared SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  add_dependencies(gtest::gtest_shared ${_EP_BASE})

  add_dependencies(am_airtree_dependencies gtest::gtest_shared)

  # IMPORTED TARGET: gtest::gtest_static
  add_library(gtest::gtest_static STATIC IMPORTED GLOBAL)
  set_target_properties(gtest::gtest_static PROPERTIES IMPORTED_LOCATION "${_STATIC_LIB}")
  target_include_directories(gtest::gtest_static SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  add_dependencies(gtest::gtest_static ${_EP_BASE})

  add_dependencies(am_airtree_dependencies gtest::gtest_static)

   # IMPORTED TARGET: gtest::gtest_main_shared
  add_library(gtest::gtest_main_shared SHARED IMPORTED GLOBAL)
  set_target_properties(gtest::gtest_main_shared PROPERTIES IMPORTED_LOCATION "${_SHARED_MAIN_LIB}")
  target_include_directories(gtest::gtest_main_shared SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  add_dependencies(gtest::gtest_main_shared ${_EP_BASE})

  add_dependencies(am_airtree_dependencies gtest::gtest_main_shared)

  # IMPORTED TARGET: gtest::gtest_main_static
  add_library(gtest::gtest_main_static STATIC IMPORTED GLOBAL)
  set_target_properties(gtest::gtest_main_static PROPERTIES IMPORTED_LOCATION "${_STATIC_MAIN_LIB}")
  target_include_directories(gtest::gtest_main_static SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  add_dependencies(gtest::gtest_main_static ${_EP_BASE})

  add_dependencies(am_airtree_dependencies gtest::gtest_main_static)

endfunction()

configure_gtest()

# Alias gtest::gtest to the shared or static lib we intend to use
if (AIRMETTLE_AIRTREE_USE_SHARED_LIBS)
  add_library(gtest::gtest ALIAS gtest::gtest_shared)
  add_library(gtest::gtest_main ALIAS gtest::gtest_main_shared)
else ()
  add_library(gtest::gtest ALIAS gtest::gtest_static)
  add_library(gtest::gtest_main ALIAS gtest::gtest_main_static)
endif ()
