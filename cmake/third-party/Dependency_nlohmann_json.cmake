include_guard(GLOBAL)

include(GNUInstallDirs)
include(ExternalProject)

function(external_configure_nlohmann_json _EP_BASE _EP_BUILD_DIR _INSTALL_DIR)
  # Remote
  set(DOWNLOAD_OPTIONS
    GIT_REPOSITORY "https://github.com/nlohmann/json.git"
    GIT_TAG "${NLOHMANN_JSON_VERSION}"
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED FALSE)

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
    -DJSON_BuildTests=OFF
    BUILD_COMMAND cmake --build . --config ${AIRMETTLE_AIRTREE_DEPS_BUILD_TYPE} -- -j${NPROC}
    INSTALL_COMMAND cmake --build . --target install
  )
  
  file(MAKE_DIRECTORY "${_INSTALL_DIR}/include")
endfunction()

function(configure_nlohmann_json)

  set(_DEPS_DIR "${AIRMETTLE_AIRTREE_DEPENDENCY_DIR}")
  set(_EP_BASE "nlohmann_json_ep")
  set(_EP_BUILD_DIR "${_DEPS_DIR}/${_EP_BASE}/build")

  airtree_dep_try_cache(DEP_NAME nlohmann_json EP_DIR "${_DEPS_DIR}/${_EP_BASE}" CACHE_HIT _cache_hit INSTALL_DIR _INSTALL_DIR)

  # Check if the nlohmann_json directory is non-empty
  file(GLOB NLOHMANN_JSON_CONTENTS "${_INSTALL_DIR}/include/*")

  if(_cache_hit)
    message(STATUS "${_EP_BASE} restored from cache.")
    add_custom_target(${_EP_BASE})
  elseif(NOT NLOHMANN_JSON_CONTENTS)
    message(STATUS "${_EP_BASE} not found at ${_INSTALL_DIR}. Will download and build it.")
    external_configure_nlohmann_json(${_EP_BASE} ${_EP_BUILD_DIR} ${_INSTALL_DIR})
    add_custom_target(clean_nlohmann_json_build ALL
      COMMAND ${CMAKE_COMMAND} -E remove_directory ${_EP_BUILD_DIR}
      COMMENT "Cleaning up nlohmann_json_build directory after installation"
    )
    add_dependencies(clean_nlohmann_json_build ${_EP_BASE})
    airtree_dep_mark_built(EP_TARGET ${_EP_BASE} INSTALL_DIR "${_INSTALL_DIR}")
  else()
    message(STATUS "${_EP_BASE} found at ${_INSTALL_DIR}. Skipping download and build.")
    add_custom_target(${_EP_BASE}) # Dummy target to keep dependencies working
  endif()

  # IMPORTED TARGET: nlohmann_json_headers
  add_library(nlohmann_json_headers INTERFACE IMPORTED GLOBAL)
  target_include_directories(nlohmann_json_headers SYSTEM INTERFACE "${_INSTALL_DIR}/include")
  add_dependencies(nlohmann_json_headers ${_EP_BASE})

  add_dependencies(am_airtree_dependencies nlohmann_json_headers)

endfunction()

configure_nlohmann_json()
