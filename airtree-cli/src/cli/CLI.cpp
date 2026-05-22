// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/cli/AirTree.hpp>
#include <airtree/cli/Logger.hpp>
#include <airtree/export/AirTreeExporter.hpp>
#include <airtree/merge/AirTreeMerge.hpp>
#include <airtree/reader/file/Reader.hpp>
#include <CLI/CLI.hpp> // for App, Option

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace airtree::cli;
using namespace airtree::reader::file;

namespace {

std::vector<char> read_binary_file(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Failed to open file: " + path);
  }
  return std::vector<char>(std::istreambuf_iterator<char>(in), {});
}

double parse_grid_bound(const std::string &s) {
  if (s == "inf" || s == "+inf") {
    return std::numeric_limits<double>::infinity();
  }
  if (s == "-inf") {
    return -std::numeric_limits<double>::infinity();
  }
  std::size_t pos = 0;
  double v;
  try {
    v = std::stod(s, &pos);
  } catch (const std::exception &) {
    throw std::invalid_argument("invalid number: '" + s + "'");
  }
  if (pos != s.size()) { // reject trailing garbage like "1abc"
    throw std::invalid_argument("invalid number: '" + s + "'");
  }
  return v;
}

// Parse one axis spec of the form "min:max:steps[:scaling]".
airtree::query::grid::GridAxisSpec parse_axis_spec(const std::string &spec) {
  std::vector<std::string> parts;
  std::stringstream ss(spec);
  std::string item;
  while (std::getline(ss, item, ':')) {
    parts.push_back(item);
  }
  // getline drops a trailing empty field, so reject a trailing ':' directly.
  if (!spec.empty() && spec.back() == ':') {
    throw std::invalid_argument(
        "axis must be 'min:max:steps[:scaling]': " + spec);
  }
  for (const auto &part : parts) {
    if (part.empty()) {
      throw std::invalid_argument(
          "axis must be 'min:max:steps[:scaling]': " + spec);
    }
  }
  if (parts.size() < 3 || parts.size() > 4) {
    throw std::invalid_argument(
        "axis must be 'min:max:steps[:scaling]': " + spec);
  }

  airtree::query::grid::GridAxisSpec ax;
  ax.min = parse_grid_bound(parts[0]);
  ax.max = parse_grid_bound(parts[1]);
  // Spec-level checks so invalid axes fail before the buffer is read. GridQuery
  // re-validates these, so it remains the authoritative source.
  if (std::isnan(ax.min) || std::isnan(ax.max)) {
    throw std::invalid_argument("min and max must not be NaN: " + spec);
  }
  if (ax.min > ax.max) {
    throw std::invalid_argument("min must be less than or equal to max: " + spec);
  }

  std::size_t pos = 0;
  long long steps;
  try {
    steps = std::stoll(parts[2], &pos);
  } catch (const std::exception &) {
    throw std::invalid_argument("invalid steps: '" + parts[2] + "'");
  }
  if (pos != parts[2].size()) { // reject trailing garbage like "4junk"
    throw std::invalid_argument("invalid steps: '" + parts[2] + "'");
  }
  if (steps <= 0) {
    throw std::invalid_argument("steps must be greater than 0: " + spec);
  }
  if (steps > static_cast<long long>(std::numeric_limits<uint32_t>::max())) {
    throw std::invalid_argument("steps too large for a 32-bit value: " + spec);
  }
  ax.steps = static_cast<uint32_t>(steps);
  ax.scaling = airtree::query::grid::GridScaling::Linear;
  if (parts.size() == 4) {
    if (parts[3] == "linear") {
      ax.scaling = airtree::query::grid::GridScaling::Linear;
    } else if (parts[3] == "mult" || parts[3] == "multiplicative") {
      ax.scaling = airtree::query::grid::GridScaling::Multiplicative;
    } else {
      throw std::invalid_argument("scaling must be linear|mult: " + spec);
    }
  }
  if (ax.scaling == airtree::query::grid::GridScaling::Multiplicative &&
      !std::isinf(ax.min) && !std::isinf(ax.max) &&
      !(ax.min > 0.0 && ax.max > ax.min)) {
    throw std::invalid_argument(
        "multiplicative scaling requires a strictly positive interior with end "
        "greater than start: " +
        spec);
  }
  return ax;
}

} // namespace

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
  int exit_code = 0;

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

  auto grid = query->add_subcommand(
      "grid", "Grid query over a 2DxP / 3DxP / "
              "4DxP buffer");
  grid->add_option("-i,--input", histogram_file, "Path to input file")
      ->required()
      ->check(CLI::ExistingFile);
  grid->add_option("-o,--output", result_file, "Path to output CSV file")
      ->required();
  std::vector<std::string> grid_axes;
  grid
      ->add_option("-a,--axis", grid_axes,
                   "Per-dimension spec 'min:max:steps[:scaling]', repeated once "
                   "per dimension. scaling = linear (default) | mult; use "
                   "inf / -inf for open ends.")
      ->required();
  uint64_t grid_max_cells = 1000000;
  grid
      ->add_option("--max-cells", grid_max_cells,
                   "Maximum number of output cells (default 1000000)")
      ->check(CLI::PositiveNumber);
  grid->callback([&]() {
    // Validate axis specs before paying the cost of reading the buffer.
    std::vector<airtree::query::grid::GridAxisSpec> axes;
    try {
      for (const auto &spec : grid_axes) {
        axes.push_back(parse_axis_spec(spec));
      }
    } catch (const std::exception &ex) {
      SPDLOG_LOGGER_ERROR(airtree::cli::logger(), "Invalid axis spec: {}",
                          ex.what());
      exit_code = 1;
      return;
    }
    AirTree airtree_cli(histogram_file, result_file);
    if (!airtree_cli.read_histogram_file()) {
      exit_code = 1;
      return;
    }
    if (!airtree_cli.grid_handler(axes, grid_max_cells)) {
      exit_code = 1;
    }
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

  auto csv = generate->add_subcommand("csv", "CSV input file");
  csv->add_option("-i,--input", input_data_file, "Path to input file")
      ->required()
      ->check(CLI::ExistingFile);
  csv->add_option("-o,--output", result_file, "Path to output file")
      ->required();
  csv
      ->add_option(
          "-s,--schema", config_name,
          std::string("Target histogram config. Supported configs are: ")
              + AirTree::get_valid_config_names())
      ->required()
      ->check(config_validator);
  std::vector<std::string> csv_column_list;
  csv
      ->add_option("-c,--columns", csv_column_list,
                   "Space separated column names")
      ->required();
  csv->add_flag(
      "-e,--e2e", e2e, "Set this flag to generate the buffer and plots.");
  csv->callback([&]() {
    AirTree airtree_cli(config_name, input_data_file, csv_column_list,
                        result_file, SUPPORTED_FILE_TYPE::AT_CSV,
                        SUPPORTED_DATA_TYPE::AT_IGNORE, e2e);
    airtree_cli.csv_handler();
  });

  std::string merge_input1;
  std::string merge_input2;
  std::string merge_output;
  auto merge = app.add_subcommand(
      "merge", "Merge two compatible histogram buffers into one");
  merge->add_option("input1", merge_input1, "First input histogram buffer")
      ->required()
      ->check(CLI::ExistingFile);
  merge->add_option("input2", merge_input2, "Second input histogram buffer")
      ->required()
      ->check(CLI::ExistingFile);
  merge->add_option("output", merge_output, "Output histogram buffer path")
      ->required();
  merge->callback([&]() {
    try {
      auto buffer1 = read_binary_file(merge_input1);
      auto buffer2 = read_binary_file(merge_input2);
      airtree::merge::mergeAirTree(buffer1, buffer2, merge_output);
      SPDLOG_LOGGER_INFO(airtree::cli::logger(),
                         "Merge complete. Output written to: {}", merge_output);
    } catch (const std::exception &ex) {
      SPDLOG_LOGGER_ERROR(airtree::cli::logger(), "Merge failed: {}", ex.what());
    }
  });

  std::string export_input;
  std::string export_output;
  bool export_parquet = false;
  bool export_csv = false;
  auto export_cmd = app.add_subcommand(
      "export", "Export a histogram buffer to Arrow / Parquet / CSV");
  export_cmd
      ->add_option("input", export_input, "Input histogram buffer (.airtree)")
      ->required()
      ->check(CLI::ExistingFile);
  auto parquet_flag = export_cmd->add_flag(
      "--parquet", export_parquet, "Export to Parquet (.parquet)");
  auto csv_flag =
      export_cmd->add_flag("--csv", export_csv, "Export to CSV (.csv)");
  parquet_flag->excludes(csv_flag);
  export_cmd->add_option("--output", export_output,
                         "Output file or directory (default: same dir as input "
                         "with matching extension)");
  export_cmd->callback([&]() {
    using airtree::xport::ExportFormat;
    ExportFormat format = ExportFormat::ARROW;
    std::string extension = ".arrow";
    if (export_parquet) {
      format = ExportFormat::PARQUET;
      extension = ".parquet";
    } else if (export_csv) {
      format = ExportFormat::CSV;
      extension = ".csv";
    }

    std::filesystem::path out_path = export_output;
    if (export_output.empty() || std::filesystem::is_directory(out_path)) {
      std::filesystem::path in(export_input);
      std::string out_filename = in.stem().string() + extension;
      out_path = export_output.empty() ? in.parent_path() / out_filename
                                       : out_path / out_filename;
    }

    try {
      auto buffer = read_binary_file(export_input);
      airtree::xport::exportAirTree(buffer, out_path.string(), format);
      SPDLOG_LOGGER_INFO(airtree::cli::logger(),
                         "Export complete. Output written to: {}",
                         out_path.string());
    } catch (const std::exception &ex) {
      SPDLOG_LOGGER_ERROR(airtree::cli::logger(), "Export failed: {}", ex.what());
    }
  });

  CLI11_PARSE(app, argc, argv);

  return exit_code;
}
