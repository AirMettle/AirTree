include_guard(GLOBAL)

# Dependency DAG registry for binary cache hashing.
# See airtree-docs/docs/developers/dependency-caching.md for the full guide
# on adding new third-party dependencies.
#
# Two properties per dep:
#   AIRTREE_DEP_DEPS_{name}        — semicolon-separated list of direct build-time dep names (empty for leaf)
#   AIRTREE_DEP_VERSION_VAR_{name} — name of the CMake variable holding the version string (from Settings.cmake)
#
# Only list DIRECT build-time dependencies (those in ExternalProject_Add DEPENDS).
# Runtime link deps that don't affect the build output should NOT be listed.

# Master list of all registered dependencies (used for count + iteration)
set(_AIRTREE_ALL_DEPS
    zlib zstd lz4 mimalloc openssl boost asio libxml2 spdlog gtest
    gbenchmark cli11 nlohmann_json flatbuffers pybind11
    curl thrift crow kafka hdf5 netcdf4 arrow
)

# --- Leaf dependencies (no build-time deps) ---
set_property(GLOBAL PROPERTY AIRTREE_DEP_DEPS_zlib          "")
set_property(GLOBAL PROPERTY AIRTREE_DEP_DEPS_zstd          "")
set_property(GLOBAL PROPERTY AIRTREE_DEP_DEPS_lz4           "")
set_property(GLOBAL PROPERTY AIRTREE_DEP_DEPS_mimalloc      "")
set_property(GLOBAL PROPERTY AIRTREE_DEP_DEPS_openssl       "")
set_property(GLOBAL PROPERTY AIRTREE_DEP_DEPS_boost         "")
set_property(GLOBAL PROPERTY AIRTREE_DEP_DEPS_asio          "")
set_property(GLOBAL PROPERTY AIRTREE_DEP_DEPS_libxml2       "")
set_property(GLOBAL PROPERTY AIRTREE_DEP_DEPS_spdlog        "")
set_property(GLOBAL PROPERTY AIRTREE_DEP_DEPS_gtest         "")
set_property(GLOBAL PROPERTY AIRTREE_DEP_DEPS_gbenchmark    "")
set_property(GLOBAL PROPERTY AIRTREE_DEP_DEPS_cli11         "")
set_property(GLOBAL PROPERTY AIRTREE_DEP_DEPS_nlohmann_json "")
set_property(GLOBAL PROPERTY AIRTREE_DEP_DEPS_flatbuffers   "")
set_property(GLOBAL PROPERTY AIRTREE_DEP_DEPS_pybind11      "")

# --- Non-leaf dependencies ---
set_property(GLOBAL PROPERTY AIRTREE_DEP_DEPS_curl      "openssl")
set_property(GLOBAL PROPERTY AIRTREE_DEP_DEPS_thrift    "openssl;boost;zlib")
set_property(GLOBAL PROPERTY AIRTREE_DEP_DEPS_crow      "boost;asio")
set_property(GLOBAL PROPERTY AIRTREE_DEP_DEPS_kafka     "zlib")
set_property(GLOBAL PROPERTY AIRTREE_DEP_DEPS_hdf5      "zlib")
set_property(GLOBAL PROPERTY AIRTREE_DEP_DEPS_netcdf4   "hdf5;curl;zlib")
set_property(GLOBAL PROPERTY AIRTREE_DEP_DEPS_arrow     "")  # Arrow vendors its own deps

# --- Version variable mapping (dep_name -> Settings.cmake variable) ---
set_property(GLOBAL PROPERTY AIRTREE_DEP_VERSION_VAR_zlib          "ZLIB_VERSION")
set_property(GLOBAL PROPERTY AIRTREE_DEP_VERSION_VAR_zstd          "ZSTD_VERSION")
set_property(GLOBAL PROPERTY AIRTREE_DEP_VERSION_VAR_lz4           "LZ4_VERSION")
set_property(GLOBAL PROPERTY AIRTREE_DEP_VERSION_VAR_mimalloc      "MIMALLOC_VERSION")
set_property(GLOBAL PROPERTY AIRTREE_DEP_VERSION_VAR_openssl       "OPENSSL_VERSION")
set_property(GLOBAL PROPERTY AIRTREE_DEP_VERSION_VAR_boost         "BOOST_VERSION")
set_property(GLOBAL PROPERTY AIRTREE_DEP_VERSION_VAR_asio          "ASIO_VERSION")
set_property(GLOBAL PROPERTY AIRTREE_DEP_VERSION_VAR_libxml2       "LIBXML2_VERSION")
set_property(GLOBAL PROPERTY AIRTREE_DEP_VERSION_VAR_spdlog        "SPDLOG_VERSION")
set_property(GLOBAL PROPERTY AIRTREE_DEP_VERSION_VAR_gtest         "GTEST_VERSION")
set_property(GLOBAL PROPERTY AIRTREE_DEP_VERSION_VAR_gbenchmark    "GOOGLE_BENCH_VERSION")
set_property(GLOBAL PROPERTY AIRTREE_DEP_VERSION_VAR_cli11         "CLI11_VERSION")
set_property(GLOBAL PROPERTY AIRTREE_DEP_VERSION_VAR_nlohmann_json "NLOHMANN_JSON_VERSION")
set_property(GLOBAL PROPERTY AIRTREE_DEP_VERSION_VAR_flatbuffers   "FLATBUFFERS_VERSION")
set_property(GLOBAL PROPERTY AIRTREE_DEP_VERSION_VAR_pybind11      "PYBIND11_VERSION")
set_property(GLOBAL PROPERTY AIRTREE_DEP_VERSION_VAR_curl          "CURL_VERSION")
set_property(GLOBAL PROPERTY AIRTREE_DEP_VERSION_VAR_thrift        "THRIFT_VERSION")
set_property(GLOBAL PROPERTY AIRTREE_DEP_VERSION_VAR_crow          "CROW_VERSION")
set_property(GLOBAL PROPERTY AIRTREE_DEP_VERSION_VAR_kafka         "LIBRDKAFKA_VERSION")
set_property(GLOBAL PROPERTY AIRTREE_DEP_VERSION_VAR_hdf5          "HDF5_VERSION")
set_property(GLOBAL PROPERTY AIRTREE_DEP_VERSION_VAR_netcdf4       "NETCDF4_VERSION")
set_property(GLOBAL PROPERTY AIRTREE_DEP_VERSION_VAR_arrow         "ARROW_VERSION")

# Total dependency count (for progress display)
list(LENGTH _AIRTREE_ALL_DEPS _AIRTREE_DEP_COUNT)
set_property(GLOBAL PROPERTY AIRTREE_DEP_TOTAL_COUNT ${_AIRTREE_DEP_COUNT})
