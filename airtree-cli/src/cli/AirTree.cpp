#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>
#include <fstream>

#include <airtree/cli/AirTree.hpp>
#include <airtree/cli/Logger.hpp>
#include <airtree/reader/file/Reader.hpp>
#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/query/AirTreeQuery_internal.hpp>
#include <iomanip>

#include <CLI/CLI.hpp> // for App, Option

using namespace airtree::reader::file;
using namespace airtree::cli;

const static std::vector<std::string> valid_configs = {
    "1DxT", "1DxF", "1DxP", "2DxP", "2DxF",
    "3DxF", "3DxP", "4DxF", "4DxP"};

AirTree::AirTree(const std::string &histogram_file,
                 const std::string &result_file)
    : input_file_(histogram_file), result_file_(result_file) {}

AirTree::AirTree(const std::string &config_name,
                 const std::string &input_data_file,
                 const std::vector<std::string> &column_list,
                 const std::string &result_file,
                 airtree::reader::file::SUPPORTED_FILE_TYPE file_type,
                 airtree::reader::file::SUPPORTED_DATA_TYPE data_type,
                 bool run_e2e)
    : config_name_(config_name), input_file_(input_data_file),
      columns_(column_list), result_file_(result_file), file_type_(file_type),
      data_type_(data_type), run_e2e_(run_e2e) {}

std::string AirTree::get_valid_config_names() {
  std::string config_list;
  for (const auto &config : valid_configs) {
    config_list += config + ", ";
  }
  // Remove the trailing ", "
  if (!config_list.empty()) {
    config_list.pop_back();
    config_list.pop_back();
  }

  return config_list;
}

bool AirTree::validate_file_type(
    airtree::reader::file::SUPPORTED_FILE_TYPE type) {
  return (type == airtree::reader::file::SUPPORTED_FILE_TYPE::AT_BINARY
          || type == airtree::reader::file::SUPPORTED_FILE_TYPE::AT_PARQUET);
}

bool AirTree::validate_data_type(
    airtree::reader::file::SUPPORTED_DATA_TYPE data_type) {
  return (data_type == airtree::reader::file::SUPPORTED_DATA_TYPE::AT_INT32
          || data_type == airtree::reader::file::SUPPORTED_DATA_TYPE::AT_INT64
          || data_type == airtree::reader::file::SUPPORTED_DATA_TYPE::AT_FLOAT
          || data_type
                 == airtree::reader::file::SUPPORTED_DATA_TYPE::AT_DOUBLE);
}

bool AirTree::validate_config_name(const std::string &config_name) {
  return std::find(valid_configs.begin(), valid_configs.end(), config_name)
         != valid_configs.end();
}

std::vector<char> AirTree::generate_buffer() {
  std::vector<char> buffer;
  uint64_t total_values = 0;
  SPDLOG_LOGGER_INFO(
      logger(), "Generating buffer for config: {}", config_name_);
  auto start = std::chrono::high_resolution_clock::now();
  if (config_name_ == "1DxT" && data_arrays_.size() == 1) {
    buffer = generate_1DxT(data_arrays_[0]);
    total_values = data_arrays_[0].length;
  } else if (config_name_ == "1DxF" && data_arrays_.size() == 1) {
    buffer = generate_1DxF(data_arrays_[0]);
    total_values = data_arrays_[0].length;
  } else if (config_name_ == "1DxP" && data_arrays_.size() == 1) {
    buffer = generate_1DxP(data_arrays_[0]);
    total_values = data_arrays_[0].length;
  } else if (config_name_ == "2DxP" && data_arrays_.size() == 2) {
    buffer = generate_2DxP(data_arrays_[0], data_arrays_[1]);
    total_values = data_arrays_[0].length + data_arrays_[1].length;
  } else if (config_name_ == "2DxF" && data_arrays_.size() == 2) {
    buffer = generate_2DxF(data_arrays_[0], data_arrays_[1]);
    total_values = data_arrays_[0].length + data_arrays_[1].length;
  } else if (config_name_ == "3DxP" && data_arrays_.size() == 3) {
    buffer = generate_3DxP(data_arrays_[0], data_arrays_[1], data_arrays_[2]);
    total_values = data_arrays_[0].length + data_arrays_[1].length
                   + data_arrays_[2].length;
  } else if (config_name_ == "3DxF" && data_arrays_.size() == 3) {
    buffer = generate_3DxF(data_arrays_[0], data_arrays_[1], data_arrays_[2]);
    total_values = data_arrays_[0].length + data_arrays_[1].length
                   + data_arrays_[2].length;
  } else if (config_name_ == "4DxP" && data_arrays_.size() == 4) {
    buffer = generate_4DxP(
        data_arrays_[0], data_arrays_[1], data_arrays_[2], data_arrays_[3]);
    total_values = data_arrays_[0].length + data_arrays_[1].length
                   + data_arrays_[2].length + data_arrays_[3].length;
  } else if (config_name_ == "4DxF" && data_arrays_.size() == 4) {
    buffer = generate_4DxF(
        data_arrays_[0], data_arrays_[1], data_arrays_[2], data_arrays_[3]);
    total_values = data_arrays_[0].length + data_arrays_[1].length
                   + data_arrays_[2].length + data_arrays_[3].length;
  } else {
    std::cerr << "Unknown config name" << std::endl;
    return {};
  }
  SPDLOG_LOGGER_INFO(logger(), "Buffer generated for config: {}", config_name_);
  auto stop = std::chrono::high_resolution_clock::now();
  SPDLOG_LOGGER_INFO(
      logger(), "Time taken to generate buffer of size {}: {} ms",
      buffer.size(),
      std::chrono::duration_cast<std::chrono::milliseconds>(stop - start)
          .count());
  auto total_values_size = total_values * sizeof(double);
  double total_values_size_mib =
      static_cast<double>(total_values_size) / (1024 * 1024);
  SPDLOG_LOGGER_INFO(logger(), "Processed {} values with total size: {} MiB",
                     total_values, total_values_size_mib);
  double throughput_mibs =
      total_values_size_mib
      / (std::chrono::duration_cast<std::chrono::milliseconds>(stop - start)
             .count()
         / 1000.0);
  SPDLOG_LOGGER_INFO(logger(), "Throughput: {} MiB/s", throughput_mibs);

  return buffer;
}

void AirTree::min_count_handler() {
  auto min_query = airtree::query::minmax::MinMax(histogram_buffer_);
  auto query_result = min_query.getMin();
  if (query_result.empty()) {
    SPDLOG_LOGGER_INFO(logger(), "No results found for minCount query.");
    return;
  }
  for (const auto &result : query_result) {
    SPDLOG_LOGGER_INFO(logger(), "Bin: ({}, {}), Count: {}",
                       result.getLowerBound(), result.getUpperBound(),
                       result.getBinCount());
  }
}

void AirTree::max_count_handler() {
  auto max_query = airtree::query::minmax::MinMax(histogram_buffer_);
  auto max_query_res = max_query.getMax();
  if (max_query_res.empty()) {
    SPDLOG_LOGGER_INFO(logger(), "No results found for maxCount query.");
    return;
  }
  for (const auto &result : max_query_res) {
    SPDLOG_LOGGER_INFO(logger(), "Bin: ({}, {}), Count: {}",
                       result.getLowerBound(), result.getUpperBound(),
                       result.getBinCount());
  }
}

void AirTree::min_value_handler() {
  auto min_query = airtree::query::minmax::MinMax(histogram_buffer_);
  auto min_query_res = min_query.getMinValue();
  if (min_query_res.empty()) {
    SPDLOG_LOGGER_INFO(logger(), "No results found for minValue query.");
    return;
  }
  for (const auto &result : min_query_res) {
    SPDLOG_LOGGER_INFO(logger(), "Bin: ({}, {}), Count: {}",
                       result.getLowerBound(), result.getUpperBound(),
                       result.getBinCount());
  }
}

void AirTree::max_value_handler() {
  auto max_query = airtree::query::minmax::MinMax(histogram_buffer_);
  auto max_query_res = max_query.getMaxValue();
  if (max_query_res.empty()) {
    SPDLOG_LOGGER_INFO(logger(), "No results found for maxValue query.");
    return;
  }
  for (const auto &result : max_query_res) {
    SPDLOG_LOGGER_INFO(logger(), "Bin: ({}, {}), Count: {}",
                       result.getLowerBound(), result.getUpperBound(),
                       result.getBinCount());
  }
}

void AirTree::topk_handler(float topk_value) {
  auto topk_query = airtree::query::topk::TopK(histogram_buffer_);
  auto query_result = topk_query.getTopK(topk_value);
  if (query_result.empty()) {
    SPDLOG_LOGGER_INFO(logger(), "No results found for topK query.");
    return;
  }
  for (const auto &result : query_result) {
    SPDLOG_LOGGER_INFO(logger(), "Bin: ({}, {}), Count: {}",
                       result.getLowerBound(), result.getUpperBound(),
                       result.getCount());
  }
}

bool AirTree::read_input_file(const std::string &file_path,
                              SUPPORTED_FILE_TYPE file_type,
                              SUPPORTED_DATA_TYPE data_type,
                              const std::vector<std::string> &columns,
                              std::vector<FPHArray> &data_arrays) {
  SPDLOG_LOGGER_INFO(logger(), "Reading input data file...");
  auto start_read = std::chrono::high_resolution_clock::now();
  input_data_vector_ = parse_file(file_path, file_type, data_type, columns);

  if (input_data_vector_.empty()) {
    SPDLOG_LOGGER_ERROR(logger(), "Error: Failed to parse the file.");
    return false;
  }

  auto build_array = [](const auto &vec) -> FPHArray {
    return buildFPHArray(vec.data(), static_cast<int>(vec.size()));
  };

  for (const auto &data : input_data_vector_) {
    std::visit(
        [&](auto &&vec) {
          data_arrays.push_back(build_array(std::forward<decltype(vec)>(vec)));
        },
        data);
  }
  SPDLOG_LOGGER_INFO(logger(), "Read complete.");
  auto stop_read = std::chrono::high_resolution_clock::now();
  SPDLOG_LOGGER_INFO(logger(), "Time taken to read input file: {} ms",
                     std::chrono::duration_cast<std::chrono::milliseconds>(
                         stop_read - start_read)
                         .count());
  // print the size of the file
  auto file_size_bytes = std::filesystem::file_size(file_path);
  double file_size_mib = static_cast<double>(file_size_bytes) / (1024 * 1024);

  SPDLOG_LOGGER_INFO(logger(), "Size of the input data file: {} MB ({} bytes)",
                     file_size_mib, file_size_bytes);
  return true;
}

bool AirTree::read_histogram_file() {
  SPDLOG_LOGGER_INFO(logger(), "Reading histogram file...");
  auto start_read = std::chrono::high_resolution_clock::now();
  std::ifstream buffer_file(input_file_, std::ios::binary);
  if (!buffer_file) {
    SPDLOG_LOGGER_ERROR(
        logger(), "Error: Could not open histogram file: {}", input_file_);
    return false;
  }
  histogram_buffer_ =
      std::vector<char>((std::istreambuf_iterator<char>(buffer_file)),
                        std::istreambuf_iterator<char>());
  buffer_file.close();
  if (histogram_buffer_.empty()) {
    SPDLOG_LOGGER_ERROR(
        logger(), "Error: Buffer file is empty or could not be read.");
    return false;
  }
  SPDLOG_LOGGER_INFO(logger(), "Buffer file read complete.");
  auto stop_read = std::chrono::high_resolution_clock::now();
  SPDLOG_LOGGER_INFO(logger(), "Time taken to read buffer file: {} ms",
                     std::chrono::duration_cast<std::chrono::milliseconds>(
                         stop_read - start_read)
                         .count());
  return true;
}

bool AirTree::write_histogram_file() {
  SPDLOG_LOGGER_INFO(logger(), "Writing histogram to file: {}", result_file_);
  std::ofstream output_file(result_file_, std::ios::binary);
  if (!output_file) {
    SPDLOG_LOGGER_ERROR(
        logger(), "Error: Could not open output file: {}", result_file_);
    return false;
  }
  output_file.write(histogram_buffer_.data(), histogram_buffer_.size());
  output_file.close();
  SPDLOG_LOGGER_INFO(logger(), "Histogram written to file successfully.");
  return true;
}

void AirTree::plot_histogram() {
  SPDLOG_LOGGER_INFO(
      logger(), "Plotting histogram for config: {}", config_name_);
  // Generate plots for the buffer
  // Call the fph_deserializer server using a syscall but
  // pass in a file path to the buffer. Now fph_deserializer
  // should work two ways - 1) if you pass in a buffer, it
  // should generate the plots and 2) if you call if with no cmd line
  // arguments, it should start the server and take you to the
  // buffer upload form.
  char command[4096];
  std::string pwd = std::filesystem::current_path().string();
  snprintf(command, sizeof(command), "fph_deserialize --buffer \"%s/%s\"",
           pwd.c_str(), result_file_.c_str());
  std::cout << "Running command: " << command << std::endl;
  int cmd_result = system(command);
  std::cout << "command result :" << cmd_result << std::endl;
}

void AirTree::binary_handler() {
  SPDLOG_LOGGER_INFO(
      logger(), "Generating histogram using given binary file...");
  if (!read_input_file(input_file_,
                       airtree::reader::file::SUPPORTED_FILE_TYPE::AT_BINARY,
                       data_type_, {}, data_arrays_)) {
    return;
  }
  histogram_buffer_ = generate_buffer();
  if (histogram_buffer_.empty()) {
    SPDLOG_LOGGER_ERROR(logger(), "Error: Histogram is empty.");
    return;
  }
  if (!write_histogram_file()) {
    return;
  }
  if (run_e2e_) {
    plot_histogram();
  }
}

void AirTree::parquet_handler() {
  SPDLOG_LOGGER_INFO(
      logger(), "Generating histogram using given parquet file...");
  if (!read_input_file(input_file_,
                       airtree::reader::file::SUPPORTED_FILE_TYPE::AT_PARQUET,
                       airtree::reader::file::SUPPORTED_DATA_TYPE::AT_IGNORE,
                       columns_, data_arrays_)) {
    return;
  }
  histogram_buffer_ = generate_buffer();
  if (histogram_buffer_.empty()) {
    SPDLOG_LOGGER_ERROR(logger(), "Error: Histogram is empty.");
    return;
  }
  if (!write_histogram_file()) {
    return;
  }
  if (run_e2e_) {
    plot_histogram();
  }
}

void AirTree::percentile_handler(float percentile_value) {
  auto percentile = query::percentile::Percentile(histogram_buffer_);
  auto percentile_result = percentile.getPercentile(percentile_value);
  if (percentile_result == -std::numeric_limits<float>::infinity()) {
    SPDLOG_LOGGER_INFO(logger(), "No results found for percentile query.");
    return;
  }
  SPDLOG_LOGGER_INFO(logger(), "Percentile: ({})", percentile_result);
}
