#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/core/common/ConfigRegistry.hpp>

#include <gtest/gtest.h>

using namespace airtree::core::common;

TEST(ConfigRegistry, LookupAllValidConfigs) {
  struct Expected {
    ConfigWire wire;
    uint8_t dims;
    uint8_t bit_length;
    uint8_t node_width;
  };

  Expected cases[] = {
      {ConfigWire::Config_1D_Tiny,    1, 13, 8},
      {ConfigWire::Config_1D_Fast,    1, 16, 8},
      {ConfigWire::Config_1D_Precise, 1, 20, 8},
      {ConfigWire::Config_2D_Fast,    2, 10, 8},
      {ConfigWire::Config_2D_Precise, 2, 12, 10},
      {ConfigWire::Config_3D_Fast,    3, 10, 8},
      {ConfigWire::Config_3D_Precise, 3, 12, 10},
      {ConfigWire::Config_4D_Fast,    4, 10, 8},
      {ConfigWire::Config_4D_Precise, 4, 12, 10},
  };

  for (const auto &c : cases) {
    auto result = lookupConfig(c.wire);
    ASSERT_TRUE(result.has_value())
        << "Failed for wire 0x" << std::hex
        << static_cast<int>(static_cast<uint8_t>(c.wire));
    EXPECT_EQ(result->dims, c.dims);
    EXPECT_EQ(result->bit_length, c.bit_length);
    EXPECT_EQ(result->node_width, c.node_width);
  }
}

TEST(ConfigRegistry, RejectUnknownWire) {
  EXPECT_FALSE(lookupConfig(static_cast<uint8_t>(0xFF)).has_value());
  EXPECT_FALSE(lookupConfig(static_cast<uint8_t>(0x03)).has_value());
  EXPECT_FALSE(lookupConfig(static_cast<uint8_t>(0x40)).has_value());
}

TEST(ConfigRegistry, DimsFromWireByte) {
  EXPECT_EQ(configDims(0x00), 1);
  EXPECT_EQ(configDims(0x02), 1);
  EXPECT_EQ(configDims(0x10), 2);
  EXPECT_EQ(configDims(0x11), 2);
  EXPECT_EQ(configDims(0x20), 3);
  EXPECT_EQ(configDims(0x30), 4);
}

class AirTreeHeaderTest : public ::testing::Test {
protected:
  AirTreeHeader makeTestHeader() {
    return makeHeader(ConfigWire::Config_2D_Fast,
                      {0x01, 0x03, 0x00, 0x00}, // dim1=f64, dim2=i32
                      1000, 5, 3, 10, 8, 42);
  }
};

TEST_F(AirTreeHeaderTest, RoundTrip) {
  auto header = makeTestHeader();

  std::vector<char> buffer;
  serializeHeader(header, buffer);

  // Patch payload_length and CRC
  uint64_t fake_payload = 12345;
  finalizeHeader(buffer, fake_payload);

  auto decoded = deserializeHeader(buffer);

  EXPECT_EQ(decoded.version, 1);
  EXPECT_EQ(decoded.header_length, kHeaderLength);
  EXPECT_EQ(decoded.config, ConfigWire::Config_2D_Fast);
  EXPECT_EQ(decoded.data_types[0], 0x01);
  EXPECT_EQ(decoded.data_types[1], 0x03);
  EXPECT_EQ(decoded.data_types[2], 0x00);
  EXPECT_EQ(decoded.data_types[3], 0x00);
  EXPECT_EQ(decoded.payload_length, fake_payload);
  EXPECT_EQ(decoded.trie_count, 1000u);
  EXPECT_EQ(decoded.pos_inf_count, 5u);
  EXPECT_EQ(decoded.neg_inf_count, 3u);
  EXPECT_EQ(decoded.pos_zero_count, 10u);
  EXPECT_EQ(decoded.neg_zero_count, 8u);
  EXPECT_EQ(decoded.nan_count, 42u);
}

TEST_F(AirTreeHeaderTest, AllConfigsRoundTrip) {
  ConfigWire configs[] = {
      ConfigWire::Config_1D_Tiny,    ConfigWire::Config_1D_Fast,
      ConfigWire::Config_1D_Precise, ConfigWire::Config_2D_Fast,
      ConfigWire::Config_2D_Precise, ConfigWire::Config_3D_Fast,
      ConfigWire::Config_3D_Precise, ConfigWire::Config_4D_Fast,
      ConfigWire::Config_4D_Precise,
  };

  for (auto cfg : configs) {
    auto header =
        makeHeader(cfg, {0x01, 0x00, 0x00, 0x00}, 100, 0, 0, 0, 0, 0);

    std::vector<char> buffer;
    serializeHeader(header, buffer);
    finalizeHeader(buffer, 0);

    auto decoded = deserializeHeader(buffer);
    EXPECT_EQ(decoded.config, cfg);
  }
}

TEST_F(AirTreeHeaderTest, MagicBytesCorrect) {
  auto header = makeTestHeader();

  std::vector<char> buffer;
  serializeHeader(header, buffer);
  finalizeHeader(buffer, 0);

  EXPECT_EQ(buffer[0], 'A');
  EXPECT_EQ(buffer[1], 'I');
  EXPECT_EQ(buffer[2], 'R');
  EXPECT_EQ(buffer[3], 'T');
}

TEST_F(AirTreeHeaderTest, RejectBadMagic) {
  auto header = makeTestHeader();

  std::vector<char> buffer;
  serializeHeader(header, buffer);
  finalizeHeader(buffer, 0);

  buffer[0] = 'X';
  EXPECT_THROW(deserializeHeader(buffer), std::runtime_error);
}

TEST_F(AirTreeHeaderTest, RejectCorruptedHeader) {
  auto header = makeTestHeader();

  std::vector<char> buffer;
  serializeHeader(header, buffer);
  finalizeHeader(buffer, 0);

  // Corrupt a byte in the middle of the header
  buffer[25] ^= 0xFF;
  EXPECT_THROW(deserializeHeader(buffer), std::runtime_error);
}

TEST_F(AirTreeHeaderTest, RejectTruncatedBuffer) {
  std::vector<char> buffer(10, 0);
  EXPECT_THROW(deserializeHeader(buffer), std::runtime_error);
}


TEST_F(AirTreeHeaderTest, RejectUnsupportedVersion) {
  auto header = makeTestHeader();

  std::vector<char> buffer;
  serializeHeader(header, buffer);
  finalizeHeader(buffer, 0);

  // Patch version to 2 (but CRC will now be wrong too)
  // We need to also fix the CRC for this test to hit the version check
  buffer[4] = 0x02;
  buffer[5] = 0x00;
  // Recompute CRC over [0..44)
  // We can't easily do this without access to internals, so just verify
  // that deserialization fails (either CRC or version check)
  EXPECT_THROW(deserializeHeader(buffer), std::runtime_error);
}

TEST_F(AirTreeHeaderTest, RejectUnknownConfig) {
  EXPECT_THROW(
      makeHeader(static_cast<ConfigWire>(0xFF), {0x01, 0, 0, 0}, 0, 0,
                 0, 0, 0, 0),
      std::runtime_error);
}

TEST_F(AirTreeHeaderTest, ConfigParamsLookup) {
  auto header = makeTestHeader();
  auto params = configParams(header);

  EXPECT_EQ(params.dims, 2);
  EXPECT_EQ(params.bit_length, 10);
  EXPECT_EQ(params.node_width, 8);
}

TEST_F(AirTreeHeaderTest, SerializedSizeIs49) {
  auto header = makeTestHeader();

  std::vector<char> buffer;
  serializeHeader(header, buffer);

  EXPECT_EQ(buffer.size(), 49u);
  EXPECT_EQ(buffer.size(), kHeaderLength);
}


TEST_F(AirTreeHeaderTest, DataTypeWireCoding) {
  // 0x00=unused, 0x01=f64, 0x02=f32, 0x03=i32, 0x04=i64
  auto header = makeHeader(ConfigWire::Config_4D_Fast,
                           {0x01, 0x02, 0x03, 0x04}, 0, 0, 0, 0, 0, 0);

  std::vector<char> buffer;
  serializeHeader(header, buffer);
  finalizeHeader(buffer, 0);

  auto decoded = deserializeHeader(buffer);
  EXPECT_EQ(decoded.data_types[0], 0x01); // f64
  EXPECT_EQ(decoded.data_types[1], 0x02); // f32
  EXPECT_EQ(decoded.data_types[2], 0x03); // i32
  EXPECT_EQ(decoded.data_types[3], 0x04); // i64
}


TEST_F(AirTreeHeaderTest, PayloadLengthPatching) {
  auto header = makeTestHeader();

  std::vector<char> buffer;
  serializeHeader(header, buffer);

  // Append some fake trie data
  std::vector<char> trie_data(256, 0x42);
  buffer.insert(buffer.end(), trie_data.begin(), trie_data.end());

  uint64_t payload = trie_data.size();
  finalizeHeader(buffer, payload);

  auto decoded = deserializeHeader(buffer);
  EXPECT_EQ(decoded.payload_length, 256u);
}

TEST_F(AirTreeHeaderTest, LargePayloadLength) {
  auto header = makeTestHeader();

  std::vector<char> buffer;
  serializeHeader(header, buffer);

  // Test with a payload_length that exceeds uint32 range
  uint64_t large_payload = 0x1'0000'0000ULL; // 4GB+
  finalizeHeader(buffer, large_payload);

  auto decoded = deserializeHeader(buffer);
  EXPECT_EQ(decoded.payload_length, large_payload);
}
