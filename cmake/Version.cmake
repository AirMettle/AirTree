include_guard(GLOBAL)

# Version strings from version.txt. include() before project() (it consumes the
# numeric form). Sets, in the caller's scope:
#   AIRTREE_FULL_VERSION     1.5.0-SNAPSHOT   --version, install prefix, package filenames
#   AIRTREE_VERSION_NUMERIC  1.5.0            project() / CPack MAJOR.MINOR.PATCH
#   AIRTREE_DISTRO_VERSION   1.5.0~SNAPSHOT   .deb/.rpm Version (tilde sorts before release)

file(STRINGS "${CMAKE_SOURCE_DIR}/version.txt" AIRTREE_FULL_VERSION)

# Validate the whole string 
if(NOT AIRTREE_FULL_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+(-[0-9A-Za-z.-]+)?$")
  message(FATAL_ERROR "version.txt: '${AIRTREE_FULL_VERSION}' is not a valid X.Y.Z[-suffix] version")
endif()
string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" AIRTREE_VERSION_NUMERIC "${AIRTREE_FULL_VERSION}")

string(REPLACE "-" "~" AIRTREE_DISTRO_VERSION "${AIRTREE_FULL_VERSION}")
