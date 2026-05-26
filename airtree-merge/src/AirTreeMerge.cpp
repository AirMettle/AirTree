// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/merge/AirTreeMerge.hpp>
#include <airtree/merge/MergeFactory.hpp>
#include <airtree/merge/Logger.hpp>
#include <airtree/core/io/AirTreeReader.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <stdexcept>
#include <fstream>

using namespace airtree::core;
using namespace airtree::core::common;
using namespace airtree::core::io;

namespace airtree::merge {

// Helper function to detect MergeConfig from AirTreeHeader config enum
MergeConfig detectConfigFromHeader(ConfigWire config) {
  switch (config) {
  case ConfigWire::Config_1D_Tiny:
    return MergeConfig::Dx1_T;
  case ConfigWire::Config_1D_Fast:
    return MergeConfig::Dx1_F;
  case ConfigWire::Config_1D_Precise:
    return MergeConfig::Dx1_P;
  case ConfigWire::Config_2D_Fast:
    return MergeConfig::Dx2_F;
  case ConfigWire::Config_2D_Precise:
    return MergeConfig::Dx2_P;
  case ConfigWire::Config_3D_Fast:
    return MergeConfig::Dx3_F;
  case ConfigWire::Config_3D_Precise:
    return MergeConfig::Dx3_P;
  case ConfigWire::Config_4D_Fast:
    return MergeConfig::Dx4_F;
  case ConfigWire::Config_4D_Precise:
    return MergeConfig::Dx4_P;
  default:
    SPDLOG_LOGGER_ERROR(logger(), "Unsupported trie configuration: 0x{:02x}",
                        static_cast<uint8_t>(config));
    throw std::runtime_error("Unsupported AirTree configuration: 0x"
                             + std::to_string(static_cast<uint8_t>(config)));
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

  if (header1.config != header2.config) {
    SPDLOG_LOGGER_ERROR(
        logger(),
        "Cannot merge buffers with different configurations: 0x{:02x} vs 0x{:02x}",
        static_cast<uint8_t>(header1.config),
        static_cast<uint8_t>(header2.config));
    throw std::runtime_error(
        "Buffer configuration mismatch: 0x"
        + std::to_string(static_cast<uint8_t>(header1.config)) + " vs 0x"
        + std::to_string(static_cast<uint8_t>(header2.config)));
  }

  SPDLOG_LOGGER_INFO(logger(), "Detected configuration: 0x{:02x}",
                     static_cast<uint8_t>(header1.config));

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
