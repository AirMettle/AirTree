#include <gtest/gtest.h>
#include <airtree/core/common/AirTreeHeader.hpp>

using namespace airtree::core::common;

TEST(HeaderDeserialization, RoundTrip1D) {
  auto header = makeHeader(ConfigWire::Config_1D_Tiny,
                           {0x01, 0x00, 0x00, 0x00},
                           500, 10, 5, 100, 80, 3);

  std::vector<char> buffer;
  serializeHeader(header, buffer);
  finalizeHeader(buffer, 0);

  auto decoded = deserializeHeader(buffer);

  EXPECT_EQ(decoded.version, 1);
  EXPECT_EQ(decoded.header_length, kHeaderLength);
  EXPECT_EQ(decoded.config, ConfigWire::Config_1D_Tiny);
  EXPECT_EQ(decoded.trie_count, 500u);
  EXPECT_EQ(decoded.pos_inf_count, 10u);
  EXPECT_EQ(decoded.neg_inf_count, 5u);
  EXPECT_EQ(decoded.pos_zero_count, 100u);
  EXPECT_EQ(decoded.neg_zero_count, 80u);
  EXPECT_EQ(decoded.nan_count, 3u);
}

TEST(HeaderDeserialization, RoundTrip2D) {
  auto header = makeHeader(ConfigWire::Config_2D_Fast,
                           {0x01, 0x02, 0x00, 0x00},
                           1000, 0, 0, 0, 0, 0);

  std::vector<char> buffer;
  serializeHeader(header, buffer);
  finalizeHeader(buffer, 0);

  auto decoded = deserializeHeader(buffer);

  EXPECT_EQ(decoded.version, 1);
  EXPECT_EQ(decoded.config, ConfigWire::Config_2D_Fast);
  EXPECT_EQ(decoded.data_types[0], 0x01);
  EXPECT_EQ(decoded.data_types[1], 0x02);
  EXPECT_EQ(decoded.trie_count, 1000u);
}

TEST(HeaderDeserialization, RoundTrip3D) {
  auto header = makeHeader(ConfigWire::Config_3D_Fast,
                           {0x01, 0x01, 0x01, 0x00},
                           2000, 1, 2, 3, 4, 5);

  std::vector<char> buffer;
  serializeHeader(header, buffer);
  finalizeHeader(buffer, 0);

  auto decoded = deserializeHeader(buffer);

  EXPECT_EQ(decoded.version, 1);
  EXPECT_EQ(decoded.config, ConfigWire::Config_3D_Fast);
  EXPECT_EQ(decoded.trie_count, 2000u);
  EXPECT_EQ(decoded.pos_inf_count, 1u);
  EXPECT_EQ(decoded.neg_inf_count, 2u);
  EXPECT_EQ(decoded.pos_zero_count, 3u);
  EXPECT_EQ(decoded.neg_zero_count, 4u);
  EXPECT_EQ(decoded.nan_count, 5u);
}

TEST(HeaderDeserialization, RoundTrip4D) {
  auto header = makeHeader(ConfigWire::Config_4D_Fast,
                           {0x01, 0x02, 0x03, 0x04},
                           3000, 0, 0, 0, 0, 0);

  std::vector<char> buffer;
  serializeHeader(header, buffer);
  finalizeHeader(buffer, 0);

  auto decoded = deserializeHeader(buffer);

  EXPECT_EQ(decoded.version, 1);
  EXPECT_EQ(decoded.config, ConfigWire::Config_4D_Fast);
  EXPECT_EQ(decoded.data_types[0], 0x01);
  EXPECT_EQ(decoded.data_types[1], 0x02);
  EXPECT_EQ(decoded.data_types[2], 0x03);
  EXPECT_EQ(decoded.data_types[3], 0x04);
  EXPECT_EQ(decoded.trie_count, 3000u);
}
