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
#include <airtree/bench/BenchPaths.hpp>
#include <airtree/bench/query/QueryFixtureBase.hpp>
#include <set>
#include <sstream>

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

// Allowed query names per schema (current airtree-query only).
std::set<std::string> allowed_queries_for_schema(const std::string &schema) {
  if (schema == "1DxT" || schema == "1DxF" || schema == "1DxP") {
    return {"topk", "minmax", "percentile", "cdf", "binboundary", "reader"};
  }
  if (schema == "2DxP" || schema == "3DxP") {
    return {"grid", "boundingbox", "binboundary", "reader"};
  }
  if (schema == "4DxP") {
    return {"grid", "binboundary", "reader"};
  }
  return {};
}

std::string query_benchmark_filter(const std::string &schema,
                                   const std::vector<std::string> &queries) {
  // Map query name -> fixture name fragment under AirTreeQuery{schema}_*
  // Registered names look like: AirTreeQuery1DxF_TopK/TopK_k5
  std::vector<std::string> parts;
  for (const auto &q : queries) {
    if (q == "topk") {
      parts.push_back("AirTreeQuery" + schema + "_TopK/.*");
    } else if (q == "minmax") {
      parts.push_back("AirTreeQuery" + schema + "_MinMax/.*");
    } else if (q == "percentile") {
      parts.push_back("AirTreeQuery" + schema + "_Percentile/.*");
    } else if (q == "grid") {
      parts.push_back("AirTreeQuery" + schema + "_Grid/.*");
    } else if (q == "boundingbox") {
      parts.push_back("AirTreeQuery" + schema + "_BoundingBox/.*");
    } else if (q == "cdf") {
      parts.push_back("AirTreeQuery" + schema + "_CDF/.*");
    } else if (q == "binboundary") {
      parts.push_back("AirTreeQuery" + schema + "_BinBoundary/.*");
    } else if (q == "reader") {
      parts.push_back("AirTreeQuery" + schema + "_Reader/.*");
    }
  }
  if (parts.empty()) {
    return "AirTreeQuery" + schema + "_.*";
  }
  std::ostringstream oss;
  for (size_t i = 0; i < parts.size(); ++i) {
    if (i > 0) {
      oss << "|";
    }
    oss << parts[i];
  }
  return oss.str();
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
  [[maybe_unused]] bool e2e = false;
  
  std::string write_airtree_path;
  
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
  
  binary->add_option(
      "--write-airtree", write_airtree_path,
      "If set, write the serialized histogram once (untimed) to this path");
  
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

    airtree::bench::BenchPaths::write_airtree_path = write_airtree_path;

    read_input_file(
        input_data_file, airtree::reader::file::SUPPORTED_FILE_TYPE::AT_BINARY, data_type_enum);

    std::vector<std::string> gb_args;
    gb_args.push_back(argv[0]);
    gb_args.push_back("--benchmark_filter="
                      + benchmark_filter_for(config_name, data_type));
    std::vector<std::string> remaining_args = app.remaining(true);
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
 
  parquet->add_option(
      "--write-airtree", write_airtree_path,
      "If set, write the serialized histogram once (untimed) to this path");
  
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
  
    airtree::bench::BenchPaths::write_airtree_path = write_airtree_path;
   
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
    std::vector<std::string> remaining_args = app.remaining(true);
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

  auto query =
      app.add_subcommand("query",
                         "Benchmark existing airtree-query APIs on a .airtree "
                         "file");

  std::string query_input;
  std::string query_schema;
  std::vector<std::string> query_list;

  query->add_option("-i,--input", query_input, "Path to .airtree")
      ->required()
      ->check(CLI::ExistingFile);
  query
      ->add_option("-s,--schema", query_schema,
                   "Schema of the buffer (must match supported suite)")
      ->required();
  query->add_option(
      "-q,--queries", query_list,
      "Subset: topk,minmax,percentile,cdf,binboundary,grid,boundingbox,reader (must be "
      "allowed for schema)");

  query->callback([&]() {
    auto allowed = allowed_queries_for_schema(query_schema);
    if (allowed.empty()) {
      throw CLI::ValidationError(
          "--schema",
          "Schema '" + query_schema
              + "' has no supported query suite in this bench build.");
    }

    std::vector<std::string> selected = query_list;
    if (selected.empty()) {
      selected.assign(allowed.begin(), allowed.end());
    } else {
      for (const auto &q : selected) {
        if (allowed.find(q) == allowed.end()) {
          throw CLI::ValidationError(
              "--queries",
              "Query '" + q + "' is not supported for schema '" + query_schema
                  + "'.");
        }
      }
    }

    airtree::bench::BenchPaths::query_schema = query_schema;
    airtree::bench::BenchPaths::query_buffer =
        airtree::bench::query::loadAirtreeFile(query_input);
    if (airtree::bench::BenchPaths::query_buffer.empty()) {
      throw CLI::ValidationError("--input", "Loaded .airtree buffer is empty");
    }

    SPDLOG_LOGGER_INFO(logger(), "Loaded .airtree ({} bytes) for schema {}",
                       airtree::bench::BenchPaths::query_buffer.size(),
                       query_schema);

    std::vector<std::string> gb_args;
    gb_args.push_back(argv[0]);
    gb_args.push_back("--benchmark_filter="
                      + query_benchmark_filter(query_schema, selected));
    std::vector<std::string> remaining_args = app.remaining(true);
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
  
  auto merge = app.add_subcommand(
      "merge", "Benchmark airtree-merge on two or more .airtree files of one schema");

  std::vector<std::string> merge_inputs;
  std::string merge_schema;

  merge
      ->add_option("-i,--inputs", merge_inputs,
                   "Two or more .airtree files (same schema); order = fold order")
      ->required()
      ->check(CLI::ExistingFile);
  merge->add_option("-s,--schema", merge_schema, "Schema label for reporting")
      ->required();

  merge->callback([&]() {
    if (merge_inputs.size() < 2) {
      throw CLI::ValidationError("--inputs", "merge needs at least two files");
    }
    airtree::bench::BenchPaths::query_schema = merge_schema;
    airtree::bench::BenchPaths::merge_buffers.clear();
    for (const auto &path : merge_inputs) {
      auto buffer = airtree::bench::query::loadAirtreeFile(path);
      if (buffer.empty()) {
        throw CLI::ValidationError("--inputs", "Loaded .airtree buffer is empty: " + path);
      }
      airtree::bench::BenchPaths::merge_buffers.push_back(std::move(buffer));
    }
    SPDLOG_LOGGER_INFO(logger(), "Loaded {} .airtree buffers for merge ({})",
                       airtree::bench::BenchPaths::merge_buffers.size(), merge_schema);

    std::vector<std::string> gb_args;
    gb_args.push_back(argv[0]);
    gb_args.push_back("--benchmark_filter=AirTreeMerge/.*");
    std::vector<std::string> remaining_args = app.remaining(true);
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
