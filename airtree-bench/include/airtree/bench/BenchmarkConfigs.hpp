// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_BENCH_INCLUDE_AIRTREE_BENCH_BENCHMARKCONFIGS_HPP
#define AIRTREE_BENCH_INCLUDE_AIRTREE_BENCH_BENCHMARKCONFIGS_HPP

#include <string>
#include <vector>
#include <algorithm>
#include <airtree/reader/file/Reader.hpp>
#include <airtree/bench/Logger.hpp>

namespace airtree::bench::configs {

// List of supported benchmark configurations.
const static std::vector<std::string> BENCH_CONFIGS = {
    "1DxT", "1DxF", "1DxP", "2DxP", "2DxF", "3DxF", "3DxP", "4DxF", "4DxP",
};

class BenchmarkConfigs {
public:
  static std::string supported_airtree_configs() {
    std::string config_list;
    for (const auto &config : BENCH_CONFIGS) {
      if (!config_list.empty()) {
        config_list += ", ";
      }
      config_list += config;
    }

    return config_list;
  }

  static bool
  validate_file_type(airtree::reader::file::SUPPORTED_FILE_TYPE type) {
    return (type == airtree::reader::file::SUPPORTED_FILE_TYPE::AT_BINARY
            || type == airtree::reader::file::SUPPORTED_FILE_TYPE::AT_PARQUET);
  }

  static bool
  validate_data_type(airtree::reader::file::SUPPORTED_DATA_TYPE data_type) {
    return (data_type == airtree::reader::file::SUPPORTED_DATA_TYPE::AT_INT32
            || data_type == airtree::reader::file::SUPPORTED_DATA_TYPE::AT_INT64
            || data_type == airtree::reader::file::SUPPORTED_DATA_TYPE::AT_FLOAT
            || data_type == airtree::reader::file::SUPPORTED_DATA_TYPE::AT_DOUBLE);
  }

  static bool validate_config_name(const std::string &config_name) {
    return std::find(BENCH_CONFIGS.begin(), BENCH_CONFIGS.end(), config_name)
           != BENCH_CONFIGS.end();
  }
};

}; // namespace airtree::bench::configs

#endif // AIRTREE_BENCH_INCLUDE_AIRTREE_BENCH_BENCHMARKCONFIGS_HPP
