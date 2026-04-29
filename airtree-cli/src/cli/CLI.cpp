#include <airtree/cli/AirTree.hpp>
#include <airtree/reader/file/Reader.hpp>
#include <CLI/CLI.hpp> // for App, Option

using namespace airtree::cli;
using namespace airtree::reader::file;

auto config_validator = CLI::Validator(
    [](std::string &input) {
      if (!AirTree::validate_config_name(input)) {
        return "Invalid schema name";
      }
      return "";
    },
    "Validates if the input schema is supported", "GenerateSchemaValidator");

auto config_validator_1D = CLI::Validator(
    [](std::string &input) {
      if (input.empty() || !AirTree::validate_config_name(input)
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
      if (!AirTree::validate_data_type(data_type_enum)) {
        return "Invalid data type";
      }
      return "";
    },
    "Validates if the input data type is supported",
    "GenerateDataTypeValidator");

int main(int argc, char *argv[]) {

  CLI::App app{"AirMettle - AirTree CLI"};
  // Register --version and -v
  app.set_version_flag(
      "-v,--version", std::string("AirMettle AirTree v") + VERSION_NUMBER);

  // Setting this to 1 since we either want to call this to generate or query
  app.require_subcommand(1);

  std::string input_data_file;
  std::string result_file;
  std::string histogram_file;
  bool e2e = false;

  auto query = app.add_subcommand("query", "Query histogram");

  auto topk = query->add_subcommand("topk", "Top K query");
  topk->add_option("-i,--input", histogram_file, "Path to input file")
      ->required()
      ->check(CLI::ExistingFile);
  topk->add_option("-o,--output", result_file, "Path to output file")
      ->required();
  double topk_value = 0.0;
  topk->add_option("-k,--topk", topk_value, "Returns Top K% bins")->required();
  topk->callback([&]() {
    AirTree airtree_cli(histogram_file, result_file);
    if (!airtree_cli.read_histogram_file()) {
      return;
    }
    airtree_cli.topk_handler(topk_value);
  });

  auto minCount =
      query->add_subcommand("min_count", "Returns bins with the minimum count");
  minCount->add_option("-i,--input", histogram_file, "Path to input file")
      ->required()
      ->check(CLI::ExistingFile);
  minCount->add_option("-o,--output", result_file, "Path to output file")
      ->required();
  minCount->callback([&]() {
    AirTree airtree_cli(histogram_file, result_file);
    if (!airtree_cli.read_histogram_file()) {
      return;
    }
    airtree_cli.min_count_handler();
  });

  auto maxCount =
      query->add_subcommand("max_count", "Returns bins with the maximum count");
  maxCount->add_option("-i,--input", histogram_file, "Path to input file")
      ->required()
      ->check(CLI::ExistingFile);
  maxCount->add_option("-o,--output", result_file, "Path to output file")
      ->required();
  maxCount->callback([&]() {
    AirTree airtree_cli(histogram_file, result_file);
    if (!airtree_cli.read_histogram_file()) {
      return;
    }
    airtree_cli.max_count_handler();
  });

  auto minValue =
      query->add_subcommand("min_value", "Returns the smallest set bin");
  minValue->add_option("-i,--input", histogram_file, "Path to input file")
      ->required()
      ->check(CLI::ExistingFile);
  minValue->add_option("-o,--output", result_file, "Path to output file")
      ->required();
  minValue->callback([&]() {
    AirTree airtree_cli(histogram_file, result_file);
    if (!airtree_cli.read_histogram_file()) {
      return;
    }
    airtree_cli.min_value_handler();
  });

  auto maxValue =
      query->add_subcommand("max_value", "Returns the largest set bin");
  maxValue->add_option("-i,--input", histogram_file, "Path to input file")
      ->required()
      ->check(CLI::ExistingFile);
  maxValue->add_option("-o,--output", result_file, "Path to output file")
      ->required();
  maxValue->callback([&]() {
    AirTree airtree_cli(histogram_file, result_file);
    if (!airtree_cli.read_histogram_file()) {
      return;
    }
    airtree_cli.max_value_handler();
  });

  float percentile_input = 0.0;
  auto percentile =
      query->add_subcommand("percentile", "Returns the percentile bin");
  percentile->add_option("-i,--input", histogram_file, "Path to input file")
      ->required()
      ->check(CLI::ExistingFile);
  percentile->add_option(
      "-p, --percentile", percentile_input, "Enter a value between [0-100].");
  percentile->add_option("-o,--output", result_file, "Path to output file")
      ->required();
  percentile->callback([&]() {
    AirTree airtree_cli(histogram_file, result_file);
    if (!airtree_cli.read_histogram_file()) {
      return;
    }
    airtree_cli.percentile_handler(percentile_input);
  });

  auto generate =
      app.add_subcommand("generate", "Generate the histogram buffer");

  generate->require_subcommand(1);

  auto parquet = generate->add_subcommand("parquet", "Parquet input file");
  parquet->add_option("-i,--input", input_data_file, "Path to input file")
      ->required()
      ->check(CLI::ExistingFile);
  parquet->add_option("-o,--output", result_file, "Path to output file")
      ->required();
  std::string config_name;
  parquet
      ->add_option(
          "-s,--schema", config_name,
          std::string("Target histogram config. Supported configs are: ")
              + AirTree::get_valid_config_names())
      ->required()
      ->check(config_validator);
  std::vector<std::string> column_list;
  parquet
      ->add_option("-c,--columns", column_list, "Space separated column names")
      ->required();
  parquet->add_flag(
      "-e,--e2e", e2e, "Set this flag to generate the buffer and plots.");
  parquet->callback([&]() {
    AirTree airtree_cli(config_name, input_data_file, column_list, result_file,
                        SUPPORTED_FILE_TYPE::AT_PARQUET,
                        SUPPORTED_DATA_TYPE::AT_IGNORE, e2e);
    airtree_cli.parquet_handler();
  });

  auto binary = generate->add_subcommand("binary", "Binary input file");
  binary->add_option("-i,--input", input_data_file, "Path to input file")
      ->required()
      ->check(CLI::ExistingFile);
  binary->add_option("-o,--output", result_file, "Path to output file")
      ->required();
  binary
      ->add_option(
          "-s,--schema", config_name,
          std::string("Target histogram config. Supported configs are: ")
              + AirTree::get_valid_config_names())
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
  binary->add_flag(
      "-e,--e2e", e2e, "Set this flag to generate the buffer and plots.");
  binary->callback([&]() {
    SUPPORTED_DATA_TYPE data_type_enum;
    if (data_type == "int32") {
      data_type_enum = SUPPORTED_DATA_TYPE::AT_INT32;
    } else if (data_type == "int64") {
      data_type_enum = SUPPORTED_DATA_TYPE::AT_INT64;
    } else if (data_type == "float") {
      data_type_enum = SUPPORTED_DATA_TYPE::AT_FLOAT;
    } else if (data_type == "double") {
      data_type_enum = SUPPORTED_DATA_TYPE::AT_DOUBLE;
    } else {
      data_type_enum = SUPPORTED_DATA_TYPE::AT_IGNORE;
    }
    AirTree airtree_cli(config_name, input_data_file, {}, result_file,
                        SUPPORTED_FILE_TYPE::AT_BINARY, data_type_enum, e2e);
    airtree_cli.binary_handler();
  });

  CLI11_PARSE(app, argc, argv);

  return 0;
}
