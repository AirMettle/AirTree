#include <airtree/core/common/ConfigWire.hpp>
#include <airtree/core/api/AirTreeGenerator.hpp>

namespace airtree::core::common {

std::optional<ConfigParams> lookupConfig(ConfigWire wire) {
  return airtree::core::api::AirTreeGeneratorRegistry::instance()
      .lookupParams(static_cast<uint8_t>(wire));
}

std::optional<ConfigParams> lookupConfig(uint8_t wire) {
  return airtree::core::api::AirTreeGeneratorRegistry::instance()
      .lookupParams(wire);
}

} // namespace airtree::core::common
