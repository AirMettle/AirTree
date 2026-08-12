include_guard(GLOBAL)

# CPack configuration. include() LAST from the root: include(CPack) must run
# after every CPACK_* var, project(), and all add_subdirectory() install rules.
# Uses AIRTREE_FULL_VERSION / AIRTREE_DISTRO_VERSION from Version.cmake.


set(_distro_id "")
if (EXISTS "/etc/os-release")
  file(STRINGS "/etc/os-release" _os_release_id REGEX "^ID=")
  string(REGEX REPLACE "^ID=\"?([^\"]*)\"?$" "\\1" _distro_id "${_os_release_id}")
endif ()
if (NOT _distro_id)
  find_program(LSB_RELEASE_COMMAND lsb_release)
  if (LSB_RELEASE_COMMAND)
    execute_process(COMMAND ${LSB_RELEASE_COMMAND} -is
        OUTPUT_VARIABLE _distro_id
        OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif ()
endif ()
string(TOLOWER "${_distro_id}" _distro_id)

if (_distro_id MATCHES "debian|ubuntu")
  set(CPACK_GENERATOR "DEB;TGZ")
elseif (_distro_id MATCHES "centos|rhel|redhat|fedora|rocky|almalinux")
  set(CPACK_GENERATOR "RPM;TGZ")
endif ()

# Set default install prefix if not specified
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/opt/airmettle/airtree/${AIRTREE_FULL_VERSION}" CACHE PATH "Install path prefix" FORCE)
endif()

# CPack configuration
set(CPACK_PACKAGE_NAME "airtree")
set(CPACK_PACKAGE_VERSION ${AIRTREE_FULL_VERSION})
set(CPACK_PACKAGE_CONTACT "support@airmettle.com")
set(CPACK_PACKAGE_DESCRIPTION "AirMettle AirTree - Hierarchical Multi Dimensional Histograms")
set(CPACK_PACKAGE_VENDOR "AirMettle")
set(CPACK_WARN_ON_ABSOLUTE_INSTALL_DESTINATION TRUE)
set(CPACK_STRIP_FILES TRUE)
set(CPACK_PACKAGE_CHECKSUM SHA256)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_DEBIAN_PACKAGE_VERSION "${AIRTREE_DISTRO_VERSION}")
set(CPACK_RPM_PACKAGE_VERSION "${AIRTREE_DISTRO_VERSION}")

include(CPack)
include(InstallRequiredSystemLibraries)
