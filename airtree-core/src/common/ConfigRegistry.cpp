#include <airtree/core/common/ConfigRegistry.hpp>

#include <array>

namespace airtree::core::common {


static constexpr std::array<ConfigParams, 9> kConfigTable = {{
    {ConfigWire::Config_1D_Tiny,       1,    13,      8,     4,    7},
    {ConfigWire::Config_1D_Fast,       1,    16,      8,     4,   10},
    {ConfigWire::Config_1D_Precise,    1,    20,      8,     5,   13},
    {ConfigWire::Config_2D_Fast,       2,    10,      8,     3,    5},
    {ConfigWire::Config_2D_Precise,    2,    12,     10,     4,    6},
    {ConfigWire::Config_3D_Fast,       3,    10,      8,     3,    5},
    {ConfigWire::Config_3D_Precise,    3,    12,     10,     4,    6},
    {ConfigWire::Config_4D_Fast,       4,    10,      8,     3,    5},
    {ConfigWire::Config_4D_Precise,    4,    12,     10,     4,    6},
}};

std::optional<ConfigParams> lookupConfig(ConfigWire wire) {
  for (const auto &entry : kConfigTable) {
    if (entry.wire == wire)
      return entry;
  }
  return std::nullopt;
}

std::optional<ConfigParams> lookupConfig(uint8_t wire) {
  return lookupConfig(static_cast<ConfigWire>(wire));
}

} // namespace airtree::core::common
