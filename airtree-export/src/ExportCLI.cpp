#include <filesystem>
#include <fstream>
#include <airtree/export/AirTreeExporter.hpp>
#include <airtree/export/Logger.hpp>

using namespace airtree::xport;

std::vector<char> readBinaryFile(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    SPDLOG_LOGGER_ERROR(logger(), "Failed to open file: {}", path);
    throw std::runtime_error("Failed to open file: " + path);
  }
  return std::vector<char>(std::istreambuf_iterator<char>(in), {});
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    SPDLOG_LOGGER_ERROR(
        logger(),
        "Usage: {} <input_file> [--parquet|--csv] [--output <output_file>]",
        argv[0]);
    return 1;
  }

  std::string input_path = argv[1];
  ExportFormat format = ExportFormat::ARROW; // Default format
  std::string output_path;

  // Parse command line arguments
  for (int i = 2; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--parquet")
      format = ExportFormat::PARQUET;
    else if (arg == "--csv")
      format = ExportFormat::CSV;
    else if (arg == "--output" && i + 1 < argc)
      output_path = argv[++i];
  }

  // Determine output path and extension
  std::filesystem::path out_path = output_path;
  if (output_path.empty() || std::filesystem::is_directory(out_path)) {
    std::filesystem::path in(input_path);
    std::string extension;

    switch (format) {
    case ExportFormat::PARQUET:
      extension = ".parquet";
      break;
    case ExportFormat::CSV:
      extension = ".csv";
      break;
    case ExportFormat::ARROW:
    default:
      extension = ".arrow";
      break;
    }

    std::string out_filename = in.stem().string() + extension;
    out_path = output_path.empty() ? in.parent_path() / out_filename
                                   : out_path / out_filename;
  }
  output_path = out_path.string();

  try {
    // Read the binary file
    std::vector<char> buffer = readBinaryFile(input_path);

    // Use the simplified export API - it handles everything internally
    exportAirTree(buffer, output_path, format);
  } catch (const std::exception &e) {
    SPDLOG_LOGGER_ERROR(logger(), "Export failed: {}", e.what());
    return 1;
  }

  return 0;
}
