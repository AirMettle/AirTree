include_guard(GLOBAL)

include(GNUInstallDirs)
include(ExternalProject)

set(CLI11_VERSION v2.3.2)

function(external_configure_cli11 _EP_BASE _EP_BUILD_DIR _INSTALL_DIR)
  set(DOWNLOAD_OPTIONS
    GIT_REPOSITORY "https://github.com/CLIUtils/CLI11.git"
    GIT_TAG "${CLI11_VERSION}"
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED FALSE)

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
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5
    BUILD_COMMAND cmake --build . --config ${AIRMETTLE_AIRTREE_DEPS_BUILD_TYPE} -- -j${NPROC}
    INSTALL_COMMAND cmake --build . --target install
  )

  file(MAKE_DIRECTORY "${_INSTALL_DIR}/include")
endfunction()

function(configure_cli11)
  set(_DEPS_DIR "${AIRMETTLE_AIRTREE_DEPENDENCY_DIR}")
  set(_EP_BASE "cli11_ep")
  set(_EP_BUILD_DIR "${_DEPS_DIR}/${_EP_BASE}/build")

  airtree_dep_try_cache(DEP_NAME cli11 EP_DIR "${_DEPS_DIR}/${_EP_BASE}" CACHE_HIT _cache_hit INSTALL_DIR _INSTALL_DIR)

  # Check if the CLI11 directory is non-empty
  file(GLOB CLI11_CONTENTS "${_INSTALL_DIR}/include/*")

  if(_cache_hit)
    message(STATUS "${_EP_BASE} restored from cache.")
    add_custom_target(${_EP_BASE})
  elseif(NOT CLI11_CONTENTS)
    message(STATUS "${_EP_BASE} not found at ${_INSTALL_DIR}. Will download and build it.")
    external_configure_cli11(${_EP_BASE} ${_EP_BUILD_DIR} ${_INSTALL_DIR})
    add_custom_target(clean_cli11_build ALL
      COMMAND ${CMAKE_COMMAND} -E remove_directory ${_EP_BUILD_DIR}
      COMMENT "Cleaning up CLI11 build directory after installation"
    )
    add_dependencies(clean_cli11_build ${_EP_BASE})
    airtree_dep_mark_built(EP_TARGET ${_EP_BASE} INSTALL_DIR "${_INSTALL_DIR}")
  else()
    message(STATUS "${_EP_BASE} found at ${_INSTALL_DIR}. Skipping download and build.")
    add_custom_target(${_EP_BASE}) # Dummy target to keep dependencies working
  endif()

# IMPORTED TARGET: cli11_headers
  add_library(cli11_headers INTERFACE IMPORTED GLOBAL)
  target_include_directories(cli11_headers SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  target_link_libraries(cli11_headers INTERFACE ${_DEPS})
  add_dependencies(cli11_headers ${_EP_BASE})

  add_dependencies(am_airtree_dependencies cli11_headers)

endfunction()

configure_cli11()
