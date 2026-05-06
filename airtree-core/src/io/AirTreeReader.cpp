#include <airtree/core/io/AirTreeReader.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/core/common/ConfigWire.hpp>
#include <airtree/core/serdes/trie1d/1DxF.hpp>
#include <airtree/core/serdes/trie1d/1DxP.hpp>
#include <airtree/core/serdes/trie1d/1DxT.hpp>
#include <airtree/core/serdes/trie2d/2DxF.hpp>
#include <airtree/core/serdes/trie2d/2DxP.hpp>
#include <airtree/core/serdes/trie3d/3DxF.hpp>
#include <airtree/core/serdes/trie3d/3DxP.hpp>
#include <airtree/core/serdes/trie4d/4DxP.hpp>
#include <airtree/core/serdes/trie4d/4DxF.hpp>
#include <airtree/core/Logger.hpp>
#include <stdexcept>

using namespace airtree::core::common;

namespace airtree::core::io {

void AirTreeReader::read(const std::vector<char> &buffer) {
  // Deserialize and validate header (magic, version, CRC, config)
  header_ = airtree::core::common::deserializeHeader(buffer);

  auto params = airtree::core::common::configParams(header_);
  dims_ = params.dims;
  bit_length_ = params.bit_length;

  // Trie data starts at header_length
  std::size_t offset = header_.header_length;

  switch (header_.config) {
  case ConfigWire::Config_1D_Tiny: {
    auto nodePtr = deserialize_1DxT(buffer, offset);
    type_ = AirTreeType(std::move(nodePtr));
  } break;
  case ConfigWire::Config_1D_Fast: {
    auto nodePtr = deserialize_1DxF(buffer, offset);
    type_ = AirTreeType(std::move(nodePtr));
  } break;
  case ConfigWire::Config_1D_Precise: {
    auto nodePtr = deserialize_1DxP(buffer, offset);
    type_ = AirTreeType(std::move(nodePtr));
  } break;
  case ConfigWire::Config_2D_Fast: {
    auto nodePtr = deserialize_2DxF(buffer, offset);
    type_ = AirTreeType(std::move(nodePtr));
  } break;
  case ConfigWire::Config_2D_Precise: {
    auto nodePtr = deserialize_2DxP(buffer, offset);
    type_ = AirTreeType(std::move(nodePtr));
  } break;
  case ConfigWire::Config_3D_Fast: {
    auto nodePtr = deserialize_3DxF(buffer, offset);
    type_ = AirTreeType(std::move(nodePtr));
  } break;
  case ConfigWire::Config_3D_Precise: {
    auto nodePtr = deserialize_3DxP(buffer, offset);
    type_ = AirTreeType(std::move(nodePtr));
  } break;
  case ConfigWire::Config_4D_Fast: {
    auto nodePtr = deserialize_4DxF(buffer, offset);
    type_ = AirTreeType(std::move(nodePtr));
  } break;
  case ConfigWire::Config_4D_Precise: {
    auto nodePtr = deserialize_4DxP(buffer, offset);
    type_ = AirTreeType(std::move(nodePtr));
  } break;
  default:
    SPDLOG_LOGGER_ERROR(logger(), "Unsupported trie configuration: 0x{:02x}",
                        static_cast<uint8_t>(header_.config));
    throw std::runtime_error("Unsupported AirTree format");
  }
}

int AirTreeReader::getDims() const noexcept { return dims_; }

int AirTreeReader::getBitLength() const noexcept { return bit_length_; }

AirTreeType AirTreeReader::getType() noexcept { return std::move(type_); }

const AirTreeHeader &AirTreeReader::getHeader() const noexcept {
  return header_;
}

} // namespace airtree::core::io