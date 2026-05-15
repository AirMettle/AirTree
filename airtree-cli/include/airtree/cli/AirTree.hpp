// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_CLI_INCLUDE_AIRTREE_CLI_AIRTREE_HPP
#define AIRTREE_CLI_INCLUDE_AIRTREE_CLI_AIRTREE_HPP

#include <airtree/cli/Logger.hpp>
#include <airtree/reader/file/Reader.hpp>
#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/query/AirTreeQuery_internal.hpp>
#include <vector>

namespace airtree::cli {

const static std::vector<std::string> valid_configs = {
    "1DxT", "1DxF", "1DxP", "2DxP", "2DxF",
    "3DxF", "3DxP", "4DxF", "4DxP"};

class AirTree {
public:
  AirTree() = default;
  // query constructor
  AirTree(const std::string &histogram_file, const std::string &result_file);
  // generate constructor
  AirTree(const std::string &config_name, const std::string &input_data_file,
          const std::vector<std::string> &column_list,
          const std::string &result_file,
          airtree::reader::file::SUPPORTED_FILE_TYPE file_type,
          airtree::reader::file::SUPPORTED_DATA_TYPE data_type, bool run_e2e);
  static std::string get_valid_config_names();
  static bool
  validate_file_type(airtree::reader::file::SUPPORTED_FILE_TYPE type);
  static bool
  validate_data_type(airtree::reader::file::SUPPORTED_DATA_TYPE type);
  static bool validate_config_name(const std::string &config_name);

  [[nodiscard]] std::vector<char> generate_buffer();
  void min_count_handler();
  void max_count_handler();
  void min_value_handler();
  void max_value_handler();
  void topk_handler(float topk_value);
  void plot_histogram();

  [[nodiscard]] bool read_histogram_file();
  [[nodiscard]] bool write_histogram_file();

  void binary_handler();
  void parquet_handler();
  void csv_handler();
  void percentile_handler(float percentile_value);

private:
  std::vector<char> histogram_buffer_;
  std::vector<std::string> columns_;
  std::string config_name_;
  std::string input_file_;
  airtree::reader::file::SUPPORTED_DATA_TYPE data_type_;
  airtree::reader::file::SUPPORTED_FILE_TYPE file_type_;
  std::string result_file_;
  bool run_e2e_;
  std::vector<FPHArray> data_arrays_;
  InputDataVector input_data_vector_;
  bool read_input_file(const std::string &file_path,
                       reader::file::SUPPORTED_FILE_TYPE file_type,
                       reader::file::SUPPORTED_DATA_TYPE data_type,
                       const std::vector<std::string> &columns,
                       std::vector<FPHArray> &data_arrays);
};

} // namespace airtree::cli

#endif // AIRTREE_CLI_INCLUDE_AIRTREE_CLI_AIRTREE_HPP