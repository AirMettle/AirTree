#include <cstddef>
#include <airtree/core/io/AirTreeReader.hpp>
#include <airtree/core/serdes/Header.hpp>
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
#include <cstring>
#include <stdexcept>
#include <iostream>

using namespace airtree::core::common;

namespace airtree::core::io {


void AirTreeReader::read(const std::vector<char> &buffer) {
  // Read the header and determine the dimensions and bit length
  std::size_t offset_ = 0;


  if (buffer.size() < sizeof(trie_header)) {
    SPDLOG_LOGGER_ERROR(logger(), "Buffer too small to read AirTree header.");
    throw std::runtime_error("Buffer too small to be valid AirTree format");
  }

  header_ = deserializeTrieHeader(buffer, offset_);

  if (std::strcmp(header_.type_code, "HierFPHG") != 0) {
    SPDLOG_LOGGER_ERROR(
        logger(), "Invalid AirTree format: {}", header_.type_code);
    throw std::runtime_error("Invalid AirTree format");
  }

  if (std::strcmp(header_.config, "113") == 0) {
    dims_ = 1;
    bit_length_ = 13;
    auto nodePtr = deserialize_1DxT(buffer, offset_);
    type_ = AirTreeType(std::move(nodePtr));
  } else if (std::strcmp(header_.config, "116") == 0) {
    dims_ = 1;
    bit_length_ = 16;
    auto nodePtr = deserialize_1DxF(buffer, offset_);
    type_ = AirTreeType(std::move(nodePtr));
  } else if (std::strcmp(header_.config, "120") == 0) {
    dims_ = 1;
    bit_length_ = 20;
    auto nodePtr = deserialize_1DxP(buffer, offset_);
    type_ = AirTreeType(std::move(nodePtr));
  } else if (std::strcmp(header_.config, "210") == 0) {
    // 2DxP
    dims_ = 2;
    bit_length_ = 12;
    auto nodePtr = deserialize_2DxP(buffer, offset_);
    type_ = AirTreeType(std::move(nodePtr));
  } else if (std::strcmp(header_.config, "288") == 0) {
    // 2DxF
    dims_ = 2;
    bit_length_ = 10;
    auto nodePtr = deserialize_2DxF(buffer, offset_);
    type_ = AirTreeType(std::move(nodePtr));
  } else if (std::strcmp(header_.config, "310") == 0) {
    // 3DxP
    dims_ = 3;
    bit_length_ = 12;
    auto nodePtr = deserialize_3DxP(buffer, offset_);
    type_ = AirTreeType(std::move(nodePtr));
  } else if (std::strcmp(header_.config, "888") == 0) {
    // 3DxF
    dims_ = 3;
    bit_length_ = 10;
    auto nodePtr = deserialize_3DxF(buffer, offset_);
    type_ = AirTreeType(std::move(nodePtr));
  } else if (std::strcmp(header_.config, "4x8") == 0) {
    // 4DxF
    dims_ = 4;
    bit_length_ = 10;
    auto nodePtr = deserialize_4DxF(buffer, offset_);
    type_ = AirTreeType(std::move(nodePtr));
  } else if (std::strcmp(header_.config, "410") == 0) {
    // 4DxP
    dims_ = 4;
    bit_length_ = 12;
    auto nodePtr = deserialize_4DxP(buffer, offset_);
    type_ = AirTreeType(std::move(nodePtr));
  } else {
    SPDLOG_LOGGER_ERROR(
        logger(), "Unsupported trie configuration: {}", header_.config);
    throw std::runtime_error("Unsupported AirTree format");
  }
}

int AirTreeReader::getDims() const noexcept {
  return dims_;
}

int AirTreeReader::getBitLength() const noexcept {
  return bit_length_;
}

AirTreeType AirTreeReader::getType() noexcept {
  return std::move(type_);
}

const trie_header &AirTreeReader::getHeader() const noexcept {
  return header_;
}


} // namespace airtree::core::io