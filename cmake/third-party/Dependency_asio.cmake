include_guard(GLOBAL)

include(GNUInstallDirs)
include(ExternalProject)

function(external_configure_asio _EP_BASE _EP_BUILD_DIR _INSTALL_DIR)
  set(DOWNLOAD_OPTIONS
    GIT_REPOSITORY "https://github.com/chriskohlhoff/asio.git"
    GIT_TAG "${ASIO_VERSION}" # Use the desired version tag
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED FALSE)

  ExternalProject_Add(
    ${_EP_BASE}
    PREFIX ${_EP_BUILD_DIR}
    ${DOWNLOAD_OPTIONS}
    EXCLUDE_FROM_ALL ON
    INSTALL_DIR ${_INSTALL_DIR}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/asio/include ${_INSTALL_DIR}/include
  )
  file(MAKE_DIRECTORY "${_INSTALL_DIR}/include")
endfunction()

function(configure_asio)
  set(_DEPS_DIR "${AIRMETTLE_AIRTREE_DEPENDENCY_DIR}")
  set(_EP_BASE "asio_ep")
  set(_EP_BUILD_DIR "${_DEPS_DIR}/${_EP_BASE}/build")

  airtree_dep_try_cache(DEP_NAME asio EP_DIR "${_DEPS_DIR}/${_EP_BASE}" CACHE_HIT _cache_hit INSTALL_DIR _INSTALL_DIR)

  if(_cache_hit)
    message(STATUS "${_EP_BASE} restored from cache.")
    add_custom_target(${_EP_BASE})
  elseif(NOT EXISTS "${_INSTALL_DIR}/include/asio.hpp")
    message(STATUS "${_EP_BASE} not found at ${_INSTALL_DIR}. Will download and install it.")
    external_configure_asio(${_EP_BASE} ${_EP_BUILD_DIR} ${_INSTALL_DIR})
    add_custom_target(clean_asio_build ALL
      COMMAND ${CMAKE_COMMAND} -E remove_directory ${_EP_BUILD_DIR}
      COMMENT "Cleaning up asio build directory after installation"
    )
    add_dependencies(clean_asio_build ${_EP_BASE})
    airtree_dep_mark_built(EP_TARGET ${_EP_BASE} INSTALL_DIR "${_INSTALL_DIR}")
  else()
    message(STATUS "${_EP_BASE} found at ${_INSTALL_DIR}. Skipping download and install.")
    add_custom_target(${_EP_BASE}) # Dummy target to keep dependencies working
  endif()

  add_library(asio::asio INTERFACE IMPORTED GLOBAL)
  set_target_properties(asio::asio PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${_INSTALL_DIR}/include"
  )
  add_dependencies(asio::asio ${_EP_BASE})
  add_dependencies(am_airtree_dependencies asio::asio)

endfunction()

configure_asio()