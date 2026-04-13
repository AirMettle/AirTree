#ifndef AIRTREE_CORE_CONFIG_REGISTRY_HPP
#define AIRTREE_CORE_CONFIG_REGISTRY_HPP

#include <cstdint>
#include <optional>

namespace airtree::core::common {


enum class ConfigWire : uint8_t {
  Config_1D_Tiny    = 0x00, // 113
  Config_1D_Fast    = 0x01, // 116
  Config_1D_Precise = 0x02, // 120
  Config_2D_Fast    = 0x10, // 210
  Config_2D_Precise = 0x11, // 212
  Config_3D_Fast    = 0x20, // 310
  Config_3D_Precise = 0x21, // 312
  Config_4D_Fast    = 0x30, // 410
  Config_4D_Precise = 0x31, // 412
};

struct ConfigParams {
  ConfigWire wire;
  uint8_t dims;
  uint8_t bit_length;
  uint8_t node_width;
  uint8_t m_width;
  uint8_t precision_bits;
};


std::optional<ConfigParams> lookupConfig(ConfigWire wire);


std::optional<ConfigParams> lookupConfig(uint8_t wire);

inline uint8_t configDims(uint8_t wire) {
  return (wire >> 4) + 1;
}

} // namespace airtree::core::common

#endif // AIRTREE_CORE_CONFIG_REGISTRY_HPP
