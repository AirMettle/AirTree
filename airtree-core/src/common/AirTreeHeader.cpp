// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/common/AirTreeHeader.hpp>

#include <array>
#include <cstring>
#include <stdexcept>
#include <string>

namespace airtree::core::common {

static constexpr auto kCrcTable = [] {
  std::array<uint32_t, 256> table{};
  for (uint32_t i = 0; i < 256; ++i) {
    uint32_t c = i;
    for (int j = 0; j < 8; ++j)
      c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
    table[i] = c;
  }
  return table;
}();

static uint32_t computeCrc32(const void *data, size_t length) {
  auto *bytes = static_cast<const uint8_t *>(data);
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < length; ++i)
    crc = kCrcTable[(crc ^ bytes[i]) & 0xFF] ^ (crc >> 8);
  return crc ^ 0xFFFFFFFFu;
}

static void writeLE16(std::vector<char> &buf, uint16_t v) {
  buf.push_back(static_cast<char>(v & 0xFF));
  buf.push_back(static_cast<char>((v >> 8) & 0xFF));
}

static void writeLE32(std::vector<char> &buf, uint32_t v) {
  buf.push_back(static_cast<char>(v & 0xFF));
  buf.push_back(static_cast<char>((v >> 8) & 0xFF));
  buf.push_back(static_cast<char>((v >> 16) & 0xFF));
  buf.push_back(static_cast<char>((v >> 24) & 0xFF));
}

static void writeLE64(std::vector<char> &buf, uint64_t v) {
  for (int i = 0; i < 8; ++i)
    buf.push_back(static_cast<char>((v >> (i * 8)) & 0xFF));
}

static uint16_t readLE16(const char *p) {
  auto *b = reinterpret_cast<const uint8_t *>(p);
  return static_cast<uint16_t>(b[0]) |
         (static_cast<uint16_t>(b[1]) << 8);
}

static uint32_t readLE32(const char *p) {
  auto *b = reinterpret_cast<const uint8_t *>(p);
  return static_cast<uint32_t>(b[0]) |
         (static_cast<uint32_t>(b[1]) << 8) |
         (static_cast<uint32_t>(b[2]) << 16) |
         (static_cast<uint32_t>(b[3]) << 24);
}

static uint64_t readLE64(const char *p) {
  auto *b = reinterpret_cast<const uint8_t *>(p);
  uint64_t v = 0;
  for (int i = 0; i < 8; ++i)
    v |= static_cast<uint64_t>(b[i]) << (i * 8);
  return v;
}

static void patchLE64(std::vector<char> &buf, size_t offset, uint64_t v) {
  for (int i = 0; i < 8; ++i)
    buf[offset + i] = static_cast<char>((v >> (i * 8)) & 0xFF);
}

static void patchLE32(std::vector<char> &buf, size_t offset, uint32_t v) {
  buf[offset + 0] = static_cast<char>(v & 0xFF);
  buf[offset + 1] = static_cast<char>((v >> 8) & 0xFF);
  buf[offset + 2] = static_cast<char>((v >> 16) & 0xFF);
  buf[offset + 3] = static_cast<char>((v >> 24) & 0xFF);
}


static constexpr size_t kOffMagic         =  0; // 4 bytes
static constexpr size_t kOffVersion       =  4; // 2 bytes
static constexpr size_t kOffHeaderLength  =  6; // 2 bytes
static constexpr size_t kOffConfig        =  8;  // 1 byte
static constexpr size_t kOffDataType      =  9; // 4 bytes
static constexpr size_t kOffPayloadLength = 13; // 8 bytes
static constexpr size_t kOffTrieCount     = 21; // 4 bytes
static constexpr size_t kOffPosInf        = 25; // 4 bytes
static constexpr size_t kOffNegInf        = 29; // 4 bytes
static constexpr size_t kOffPosZero       = 33; // 4 bytes
static constexpr size_t kOffNegZero       = 37; // 4 bytes
static constexpr size_t kOffNan           = 41; // 4 bytes
[[maybe_unused]] static constexpr size_t kOffChecksum      = 45; // 4 bytes


AirTreeHeader makeHeader(ConfigWire config,
                         std::array<uint8_t, 4> data_types,
                         uint32_t trie_count, uint32_t pos_inf_count,
                         uint32_t neg_inf_count, uint32_t pos_zero_count,
                         uint32_t neg_zero_count, uint32_t nan_count) {
  if (!lookupConfig(config))
    throw std::runtime_error("makeHeader: unknown config wire value");

  AirTreeHeader h;
  h.version = 1;
  h.header_length = kHeaderLength;
  h.config = config;
  h.data_types = data_types;
  h.payload_length = 0; // patched after trie serialization
  h.trie_count = trie_count;
  h.pos_inf_count = pos_inf_count;
  h.neg_inf_count = neg_inf_count;
  h.pos_zero_count = pos_zero_count;
  h.neg_zero_count = neg_zero_count;
  h.nan_count = nan_count;
  return h;
}


void serializeHeader(const AirTreeHeader &header,
                     std::vector<char> &buffer) {
  size_t start = buffer.size();
  buffer.reserve(start + kHeaderLength);

  // magic (4 bytes)
  buffer.insert(buffer.end(), kMagic, kMagic + 4);

  // version (uint16 LE)
  writeLE16(buffer, header.version);

  // header_length (uint16 LE)
  writeLE16(buffer, header.header_length);

  // config (uint8)
  buffer.push_back(static_cast<char>(static_cast<uint8_t>(header.config)));

  // data_type[4]
  for (auto dt : header.data_types)
    buffer.push_back(static_cast<char>(dt));

  // payload_length (uint64 LE) — placeholder, patched later
  writeLE64(buffer, header.payload_length);

  // trie_count (uint32 LE)
  writeLE32(buffer, header.trie_count);

  // special counts (5 x uint32 LE)
  writeLE32(buffer, header.pos_inf_count);
  writeLE32(buffer, header.neg_inf_count);
  writeLE32(buffer, header.pos_zero_count);
  writeLE32(buffer, header.neg_zero_count);
  writeLE32(buffer, header.nan_count);

  // CRC-32 placeholder (zeroed — patched by finalizeHeader)
  writeLE32(buffer, 0);
}

AirTreeHeader deserializeHeader(const std::vector<char> &buffer) {
  if (buffer.size() < kHeaderLength)
    throw std::runtime_error(
        "Buffer too small to contain AirTree header");

  // Verify magic
  if (std::memcmp(buffer.data() + kOffMagic, kMagic, 4) != 0)
    throw std::runtime_error("Invalid AirTree magic bytes");

  // Read version
  uint16_t version = readLE16(buffer.data() + kOffVersion);
  if (version > 1)
    throw std::runtime_error(
        "Unsupported AirTree version: " + std::to_string(version));

  // Read header_length
  uint16_t header_length = readLE16(buffer.data() + kOffHeaderLength);
  if (header_length < kHeaderLength)
    throw std::runtime_error("header_length smaller than v1 minimum");
  if (buffer.size() < header_length)
    throw std::runtime_error(
        "Buffer too small for declared header_length");

  // Verify CRC-32 over [0 .. header_length - 4)
  uint32_t stored_crc = readLE32(buffer.data() + header_length - 4);
  uint32_t computed_crc =
      computeCrc32(buffer.data(), header_length - 4);
  if (stored_crc != computed_crc)
    throw std::runtime_error("Header CRC-32 mismatch");

  auto wire = static_cast<uint8_t>(buffer[kOffConfig]);
  if (!lookupConfig(wire))
    throw std::runtime_error(
        "Unknown config wire value: 0x" +
        std::to_string(static_cast<unsigned>(wire)));

  // Populate struct
  AirTreeHeader h;
  h.version = version;
  h.header_length = header_length;
  h.config = static_cast<ConfigWire>(wire);

  for (int i = 0; i < 4; ++i)
    h.data_types[i] = static_cast<uint8_t>(buffer[kOffDataType + i]);

  h.payload_length = readLE64(buffer.data() + kOffPayloadLength);
  h.trie_count = readLE32(buffer.data() + kOffTrieCount);
  h.pos_inf_count = readLE32(buffer.data() + kOffPosInf);
  h.neg_inf_count = readLE32(buffer.data() + kOffNegInf);
  h.pos_zero_count = readLE32(buffer.data() + kOffPosZero);
  h.neg_zero_count = readLE32(buffer.data() + kOffNegZero);
  h.nan_count = readLE32(buffer.data() + kOffNan);

  return h;
}


void finalizeHeader(std::vector<char> &buffer,
                                   uint64_t payload_length) {
  if (buffer.size() < kHeaderLength)
    throw std::runtime_error(
        "Buffer too small to patch header");

  uint16_t header_length = readLE16(buffer.data() + kOffHeaderLength);

  // Patch payload_length at its fixed offset
  patchLE64(buffer, kOffPayloadLength, payload_length);

  // Recompute CRC-32 over [0 .. header_length - 4) and write it
  uint32_t crc = computeCrc32(buffer.data(), header_length - 4);
  patchLE32(buffer, header_length - 4, crc);
}

ConfigParams configParams(const AirTreeHeader &header) {
  auto result = lookupConfig(header.config);
  if (!result)
    throw std::runtime_error("configParams: unknown config in header");
  return *result;
}

} // namespace airtree::core::common
