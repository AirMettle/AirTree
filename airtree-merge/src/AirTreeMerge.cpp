#include <airtree/merge/AirTreeMerge.hpp>
#include <airtree/merge/MergeFactory.hpp>
#include <airtree/merge/Logger.hpp>
#include <airtree/core/io/AirTreeReader.hpp>
#include <cstring>
#include <stdexcept>
#include <fstream>

using namespace airtree::core::common;
using namespace airtree::core::io;

namespace airtree::merge {

// Helper function to detect MergeConfig from trie header config string
MergeConfig detectConfigFromHeader(const char *config) {
  if (std::strcmp(config, "113") == 0) {
    return MergeConfig::Dx1_T;
  } else if (std::strcmp(config, "116") == 0) {
    return MergeConfig::Dx1_F;
  } else if (std::strcmp(config, "120") == 0) {
    return MergeConfig::Dx1_P;
  } else if (std::strcmp(config, "288") == 0) {
    return MergeConfig::Dx2_F;
  } else if (std::strcmp(config, "210") == 0) {
    return MergeConfig::Dx2_P;
  } else if (std::strcmp(config, "888") == 0) {
    return MergeConfig::Dx3_F;
  } else if (std::strcmp(config, "310") == 0) {
    return MergeConfig::Dx3_P;
  } else if (std::strcmp(config, "4x8") == 0) {
    return MergeConfig::Dx4_F;
  } else if (std::strcmp(config, "410") == 0) {
    return MergeConfig::Dx4_P;
  } else {
    SPDLOG_LOGGER_ERROR(logger(), "Unsupported trie configuration: {}", config);
    throw std::runtime_error("Unsupported AirTree configuration: "
                             + std::string(config));
  }
}

std::vector<char> mergeAirTree(const std::vector<char> &buffer1,
                               const std::vector<char> &buffer2) {
  SPDLOG_LOGGER_INFO(logger(), "Starting AirTree merge operation");
  SPDLOG_LOGGER_INFO(logger(), "Buffer 1 size: {} bytes", buffer1.size());
  SPDLOG_LOGGER_INFO(logger(), "Buffer 2 size: {} bytes", buffer2.size());

  // Use AirTreeReader to parse and validate both buffers
  AirTreeReader reader1, reader2;

  try {
    reader1.read(buffer1);
  } catch (const std::exception &e) {
    SPDLOG_LOGGER_ERROR(logger(), "Failed to read buffer 1: {}", e.what());
    throw std::runtime_error("Invalid AirTree buffer 1: "
                             + std::string(e.what()));
  }

  try {
    reader2.read(buffer2);
  } catch (const std::exception &e) {
    SPDLOG_LOGGER_ERROR(logger(), "Failed to read buffer 2: {}", e.what());
    throw std::runtime_error("Invalid AirTree buffer 2: "
                             + std::string(e.what()));
  }

  // Get headers and validate that both buffers have the same configuration
  const auto &header1 = reader1.getHeader();
  const auto &header2 = reader2.getHeader();

  if (std::strcmp(header1.config, header2.config) != 0) {
    SPDLOG_LOGGER_ERROR(
        logger(),
        "Cannot merge buffers with different configurations: {} vs {}",
        header1.config, header2.config);
    throw std::runtime_error("Buffer configuration mismatch: "
                             + std::string(header1.config) + " vs "
                             + std::string(header2.config));
  }

  SPDLOG_LOGGER_INFO(logger(), "Detected configuration: {}", header1.config);

  // Detect MergeConfig and perform merge
  MergeConfig config = detectConfigFromHeader(header1.config);
  auto strategy = MergeFactory::create(config);
  auto mergedBuffer = strategy->merge(buffer1, buffer2);

  SPDLOG_LOGGER_INFO(logger(),
                     "Merge completed successfully. Output size: {} bytes",
                     mergedBuffer.size());

  return mergedBuffer;
}

void mergeAirTree(const std::vector<char> &buffer1,
                  const std::vector<char> &buffer2,
                  const std::string &output_path) {
  // Perform merge
  auto mergedBuffer = mergeAirTree(buffer1, buffer2);

  // Write to file
  SPDLOG_LOGGER_INFO(logger(), "Writing merged result to: {}", output_path);
  std::ofstream outFile(output_path, std::ios::binary);
  if (!outFile) {
    SPDLOG_LOGGER_ERROR(
        logger(), "Failed to open output file: {}", output_path);
    throw std::runtime_error("Failed to open output file: " + output_path);
  }

  outFile.write(mergedBuffer.data(), mergedBuffer.size());
  outFile.close();

  if (!outFile) {
    SPDLOG_LOGGER_ERROR(
        logger(), "Failed to write to output file: {}", output_path);
    throw std::runtime_error("Failed to write to output file: " + output_path);
  }

  SPDLOG_LOGGER_INFO(logger(),
                     "Merge completed successfully. Output written to: {}",
                     output_path);
}

} // namespace airtree::merge
