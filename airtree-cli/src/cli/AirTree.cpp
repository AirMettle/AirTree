// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <limits>
#include <string>
#include <tuple>
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
    : columns_(column_list), config_name_(config_name),
      input_file_(input_data_file), data_type_(data_type), file_type_(file_type),
      result_file_(result_file), run_e2e_(run_e2e) {}

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
          || type == airtree::reader::file::SUPPORTED_FILE_TYPE::AT_PARQUET
          || type == airtree::reader::file::SUPPORTED_FILE_TYPE::AT_CSV);
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

bool AirTree::write_bins_csv(
    const std::string &query_name,
    const std::vector<std::tuple<double, double, uint64_t>> &rows) {
  std::ofstream out(result_file_);
  if (!out) {
    SPDLOG_LOGGER_ERROR(
        logger(), "Error: Could not open output file: {}", result_file_);
    return false;
  }
  // Full precision so bin boundaries round-trip exactly.
  out << std::setprecision(std::numeric_limits<double>::max_digits10);
  out << "lower,upper,count\n";
  for (const auto &[lower, upper, count] : rows) {
    out << lower << "," << upper << "," << count << "\n";
  }
  out.close();
  if (!out) { // catches write/flush failures (e.g. disk full)
    SPDLOG_LOGGER_ERROR(
        logger(), "Error: Failed to write output file: {}", result_file_);
    return false;
  }
  SPDLOG_LOGGER_INFO(logger(), "{} complete: {} bins written to {}", query_name,
                     rows.size(), result_file_);
  return true;
}

bool AirTree::min_count_handler() {
  auto min_query = airtree::query::minmax::MinMax(histogram_buffer_);
  auto query_result = min_query.getMin();
  std::vector<std::tuple<double, double, uint64_t>> rows;
  rows.reserve(query_result.size());
  for (const auto &result : query_result) {
    rows.emplace_back(result.getLowerBound(), result.getUpperBound(),
                      result.getBinCount());
  }
  return write_bins_csv("min_count query", rows);
}

bool AirTree::max_count_handler() {
  auto max_query = airtree::query::minmax::MinMax(histogram_buffer_);
  auto max_query_res = max_query.getMax();
  std::vector<std::tuple<double, double, uint64_t>> rows;
  rows.reserve(max_query_res.size());
  for (const auto &result : max_query_res) {
    rows.emplace_back(result.getLowerBound(), result.getUpperBound(),
                      result.getBinCount());
  }
  return write_bins_csv("max_count query", rows);
}

bool AirTree::min_value_handler() {
  auto min_query = airtree::query::minmax::MinMax(histogram_buffer_);
  auto min_query_res = min_query.getMinValue();
  std::vector<std::tuple<double, double, uint64_t>> rows;
  rows.reserve(min_query_res.size());
  for (const auto &result : min_query_res) {
    rows.emplace_back(result.getLowerBound(), result.getUpperBound(),
                      result.getBinCount());
  }
  return write_bins_csv("min_value query", rows);
}

bool AirTree::max_value_handler() {
  auto max_query = airtree::query::minmax::MinMax(histogram_buffer_);
  auto max_query_res = max_query.getMaxValue();
  std::vector<std::tuple<double, double, uint64_t>> rows;
  rows.reserve(max_query_res.size());
  for (const auto &result : max_query_res) {
    rows.emplace_back(result.getLowerBound(), result.getUpperBound(),
                      result.getBinCount());
  }
  return write_bins_csv("max_value query", rows);
}

bool AirTree::topk_handler(float topk_value) {
  auto topk_query = airtree::query::topk::TopK(histogram_buffer_);
  auto query_result = topk_query.getTopK(topk_value);
  std::vector<std::tuple<double, double, uint64_t>> rows;
  rows.reserve(query_result.size());
  for (const auto &result : query_result) {
    rows.emplace_back(result.getLowerBound(), result.getUpperBound(),
                      result.getCount());
  }
  return write_bins_csv("topk query", rows);
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

void AirTree::csv_handler() {
  SPDLOG_LOGGER_INFO(
      logger(), "Generating histogram using given CSV file...");
  if (!read_input_file(input_file_,
                       airtree::reader::file::SUPPORTED_FILE_TYPE::AT_CSV,
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

bool AirTree::percentile_handler(float percentile_value) {
  auto percentile = query::percentile::Percentile(histogram_buffer_);
  auto percentile_result = percentile.getPercentile(percentile_value);
  if (percentile_result == -std::numeric_limits<float>::infinity()) {
    SPDLOG_LOGGER_INFO(logger(), "No results found for percentile query.");
  }

  std::ofstream out(result_file_);
  if (!out) {
    SPDLOG_LOGGER_ERROR(
        logger(), "Error: Could not open output file: {}", result_file_);
    return false;
  }
  out << std::setprecision(std::numeric_limits<double>::max_digits10);
  out << "percentile,value\n";
  out << percentile_value << "," << percentile_result << "\n";
  out.close();
  if (!out) { 
    SPDLOG_LOGGER_ERROR(
        logger(), "Error: Failed to write output file: {}", result_file_);
    return false;
  }
  SPDLOG_LOGGER_INFO(logger(), "percentile query complete: written to {}",
                     result_file_);
  return true;
}

bool AirTree::grid_handler(
    const std::vector<airtree::query::grid::GridAxisSpec> &axes,
    uint64_t max_cells) {
  using namespace airtree::query::grid;

  GridResult result;
  try {
    GridQuery grid(histogram_buffer_);
    result = grid.getGrid(axes, max_cells);
  } catch (const std::exception &ex) {
    SPDLOG_LOGGER_ERROR(logger(), "Grid query failed: {}", ex.what());
    return false;
  }

  std::ofstream out(result_file_);
  if (!out) {
    SPDLOG_LOGGER_ERROR(
        logger(), "Error: Could not open output file: {}", result_file_);
    return false;
  }
  // Full precision so snapped internal bin boundaries round-trip exactly.
  out << std::setprecision(std::numeric_limits<double>::max_digits10);

  auto kind_str = [](IntervalKind k) -> const char * {
    switch (k) {
    case IntervalKind::NegInf:
      return "-inf";
    case IntervalKind::PosInf:
      return "+inf";
    case IntervalKind::Finite:
    default:
      return "finite";
    }
  };

  // CSV header: the full interval semantics per dimension, plus the count.
  for (uint16_t d = 0; d < result.dims; ++d) {
    out << "dim" << d << "_min,dim" << d << "_max,dim" << d
        << "_min_inclusive,dim" << d << "_max_inclusive,dim" << d << "_kind,dim"
        << d << "_is_edge,";
  }
  out << "count\n";

  // Stream one row per cell, decoding the flat row-major index back into a
  // per-axis partition index. Done in place to avoid materializing every cell.
  auto shape = result.shape();
  for (std::size_t idx = 0; idx < result.counts.size(); ++idx) {
    std::size_t rem = idx;
    std::array<std::size_t, 4> p{};
    for (std::size_t d = shape.size(); d-- > 0;) {
      std::size_t n = shape[d];
      p[d] = (n == 0) ? 0 : rem % n;
      rem = (n == 0) ? 0 : rem / n;
    }
    for (uint16_t d = 0; d < result.dims; ++d) {
      const auto &iv = result.axis_intervals[d][p[d]];
      out << iv.lower << "," << iv.upper << ","
          << (iv.lower_inclusive ? "true" : "false") << ","
          << (iv.upper_inclusive ? "true" : "false") << "," << kind_str(iv.kind)
          << "," << (iv.is_edge ? "true" : "false") << ",";
    }
    out << result.counts[idx] << "\n";
  }
  out.close();
  if (!out) { // catches write/flush failures (e.g. disk full)
    SPDLOG_LOGGER_ERROR(
        logger(), "Error: Failed to write output file: {}", result_file_);
    return false;
  }
  SPDLOG_LOGGER_INFO(logger(), "Grid query complete: {} cells written to {}",
                     result.counts.size(), result_file_);
  return true;
}
