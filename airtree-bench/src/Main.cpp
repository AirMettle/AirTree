// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <CLI/Validators.hpp>
#include <airtree/core/common/FPHArray.hpp>
#include <CLI/CLI.hpp>
#include <cctype>
#include <vector>
#include <airtree/bench/BenchmarkData.hpp>
#include <airtree/bench/BenchmarkConfigs.hpp>
#include <airtree/reader/file/Reader.hpp>
#include <airtree/bench/Logger.hpp>
#include <benchmark/benchmark.h>

using namespace airtree::reader::file;
using namespace airtree::bench::configs;

void read_input_file(const std::string &file_path,
                     SUPPORTED_FILE_TYPE file_type,
                     SUPPORTED_DATA_TYPE data_type,
                     const std::vector<std::string> &columns = {}) {
  SPDLOG_LOGGER_INFO(logger(), "Reading input data file...");
  InputDataVector input_data_vector =
      parse_file(file_path, file_type, data_type, columns);

  if (input_data_vector.empty()) {
    SPDLOG_LOGGER_ERROR(logger(), "Error: Failed to parse the file.");
    return;
  }

  for (const auto &data : input_data_vector) {
    std::visit(
        [&](auto &&vec) {
          using VecType = std::decay_t<decltype(vec)>;
          using T = typename VecType::value_type;

          airtree::bench::BenchmarkData<T>::raw_data.emplace_back(vec);

          const auto &persistent_vec =
              airtree::bench::BenchmarkData<T>::raw_data.back();
          airtree::bench::BenchmarkData<T>::fpharrays.emplace_back(
              FPHArray(persistent_vec));
        },
        data);
  }
  SPDLOG_LOGGER_INFO(logger(), "Read complete.");

  // print the size of the file
  auto file_size_bytes = std::filesystem::file_size(file_path);
  double file_size_mib = static_cast<double>(file_size_bytes) / (1024 * 1024);

  SPDLOG_LOGGER_INFO(logger(), "Size of the input data file: {} MB ({} bytes)",
                     file_size_mib, file_size_bytes);
}

auto config_validator = CLI::Validator(
    [](std::string &input) {
      if (!BenchmarkConfigs::validate_config_name(input)) {
        return "Invalid schema name";
      }
      return "";
    },
    "Validates if the input schema is supported", "GenerateSchemaValidator");

auto config_validator_1D = CLI::Validator(
    [](std::string &input) {
      if (input.empty() || !BenchmarkConfigs::validate_config_name(input)
          || input[0] != '1') {
        return "Invalid schema name or not a 1D schema";
      }
      return "";
    },
    "Validates if the input schema is a supported 1D config",
    "Generate1DSchemaValidator");

auto data_type_validator = CLI::Validator(
    [](std::string &input) {
      airtree::reader::file::SUPPORTED_DATA_TYPE data_type_enum;
      if (input == "int32") {
        data_type_enum = airtree::reader::file::SUPPORTED_DATA_TYPE::AT_INT32;
      } else if (input == "int64") {
        data_type_enum = airtree::reader::file::SUPPORTED_DATA_TYPE::AT_INT64;
      } else if (input == "float") {
        data_type_enum = airtree::reader::file::SUPPORTED_DATA_TYPE::AT_FLOAT;
      } else if (input == "double") {
        data_type_enum = airtree::reader::file::SUPPORTED_DATA_TYPE::AT_DOUBLE;
      } else {
        data_type_enum = airtree::reader::file::SUPPORTED_DATA_TYPE::AT_IGNORE;
      }
      if (!BenchmarkConfigs::validate_data_type(data_type_enum)) {
        return "Invalid data type";
      }
      return "";
    },
    "Validates if the input data type is supported",
    "GenerateDataTypeValidator");

std::string benchmark_filter_for(const std::string &config_name,
                                 const std::string &data_type) {
  return "AirTreeBench" + config_name + "/.*" + data_type;
}

int main(int argc, char *argv[]) {

  CLI::App app{"AirMettle - AirTree CLI"};
  app.failure_message(CLI::FailureMessage::help);
  // Allow unknown options to be passed through to Google Benchmark
  app.allow_extras();
  app.set_version_flag(
      "-v,--version", std::string("AirMettle AirTree v") + VERSION_NUMBER);

  app.require_subcommand(1);

  std::string input_data_file;
  std::string result_file;
  std::string histogram_file;
  std::string config_name;
  bool e2e = false;

  auto generate =
      app.add_subcommand("generate", "Generate the histogram buffer");

  generate->require_subcommand(1);

  auto binary = generate->add_subcommand("binary", "Binary input file");
  binary->add_option("-i,--input", input_data_file, "Path to input file")
      ->required()
      ->check(CLI::ExistingFile);
  binary
      ->add_option(
          "-s,--schema", config_name,
          std::string("Target config to benchmark. Supported configs are: ")
              + BenchmarkConfigs::supported_airtree_configs())
      ->required()
      ->check(config_validator)
      ->check(config_validator_1D);
  std::string data_type;
  binary
      ->add_option("-d,--data-type", data_type,
                   "Data type of the binary file. Supported types are: int32, "
                   "int64, float, double.")
      ->required()
      ->check(data_type_validator);

  binary->callback([&]() {
    airtree::reader::file::SUPPORTED_DATA_TYPE data_type_enum;
    if (data_type == "int32") {
      data_type_enum = airtree::reader::file::SUPPORTED_DATA_TYPE::AT_INT32;
    } else if (data_type == "int64") {
      data_type_enum = airtree::reader::file::SUPPORTED_DATA_TYPE::AT_INT64;
    } else if (data_type == "float") {
      data_type_enum = airtree::reader::file::SUPPORTED_DATA_TYPE::AT_FLOAT;
    } else if (data_type == "double") {
      data_type_enum = airtree::reader::file::SUPPORTED_DATA_TYPE::AT_DOUBLE;
    } else {
      data_type_enum = airtree::reader::file::SUPPORTED_DATA_TYPE::AT_IGNORE;
    }

    read_input_file(
        input_data_file, airtree::reader::file::SUPPORTED_FILE_TYPE::AT_BINARY, data_type_enum);

    std::vector<std::string> gb_args;
    gb_args.push_back(argv[0]);
    gb_args.push_back("--benchmark_filter="
                      + benchmark_filter_for(config_name, data_type));
    std::vector<std::string> remaining_args = app.remaining();
    for (const auto &arg : remaining_args) {
      gb_args.push_back(arg);
    }

    std::vector<char *> gb_argv;
    for (auto &str : gb_args) {
      gb_argv.push_back(str.data());
    }

    int gb_argc = static_cast<int>(gb_argv.size());

    ::benchmark::Initialize(&gb_argc, gb_argv.data());
    if (::benchmark::ReportUnrecognizedArguments(gb_argc, gb_argv.data()))
      return;
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
  });

  auto parquet = generate->add_subcommand("parquet", "Parquet input file");
  parquet->add_option("-i,--input", input_data_file, "Path to input file")
      ->required()
      ->check(CLI::ExistingFile);
  parquet
      ->add_option(
          "-s,--schema", config_name,
          std::string("Target histogram config. Supported configs are: ")
              + BenchmarkConfigs::supported_airtree_configs())
      ->required()
      ->check(config_validator);
  std::vector<std::string> column_list;
  parquet
      ->add_option("-c,--columns", column_list, "Space separated column names")
      ->required();
  parquet->parse_complete_callback([&]() {
    if (config_name.empty() || !std::isdigit(config_name[0])) {
      return;
    }

    size_t expected_columns = config_name[0] - '0';
    if (column_list.size() != expected_columns) {
      throw CLI::ValidationError(
          "--columns",
          "The " + config_name + " schema requires exactly "
              + std::to_string(expected_columns) + " column(s), but "
              + std::to_string(column_list.size()) + " were provided.");
    }
  });
  parquet->callback([&]() {
    read_input_file(input_data_file, airtree::reader::file::SUPPORTED_FILE_TYPE::AT_PARQUET,
                    airtree::reader::file::SUPPORTED_DATA_TYPE::AT_IGNORE,
                    column_list);

    std::string loaded_type = "unknown";
    if (!airtree::bench::BenchmarkData<double>::raw_data.empty())
      loaded_type = "double";
    else if (!airtree::bench::BenchmarkData<float>::raw_data.empty())
      loaded_type = "float";
    else if (!airtree::bench::BenchmarkData<int32_t>::raw_data.empty())
      loaded_type = "int32";
    else if (!airtree::bench::BenchmarkData<int64_t>::raw_data.empty())
      loaded_type = "int64";

    std::vector<std::string> gb_args;
    gb_args.push_back(argv[0]);
    gb_args.push_back("--benchmark_filter="
                      + benchmark_filter_for(config_name, loaded_type));
    std::vector<std::string> remaining_args = app.remaining();
    for (const auto &arg : remaining_args) {
      gb_args.push_back(arg);
    }

    std::vector<char *> gb_argv;
    for (auto &str : gb_args) {
      gb_argv.push_back(str.data());
    }

    int gb_argc = static_cast<int>(gb_argv.size());

    ::benchmark::Initialize(&gb_argc, gb_argv.data());
    if (::benchmark::ReportUnrecognizedArguments(gb_argc, gb_argv.data()))
      return;
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
  });

  CLI11_PARSE(app, argc, argv);

  return 0;
}
