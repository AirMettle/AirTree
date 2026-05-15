// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/api/AirTreeGenerator.hpp>
#include <airtree/core/AirTreeCore_internal.hpp>

using namespace airtree::core::api;
using namespace airtree::core::common;
using namespace airtree::core::schema::trie1d;
using namespace airtree::core::schema::trie2d;
using namespace airtree::core::schema::trie3d;
using namespace airtree::core::schema::trie4d;

AirTreeGeneratorRegistry &AirTreeGeneratorRegistry::instance() {
  static AirTreeGeneratorRegistry kInstance;
  return kInstance;
}

void AirTreeGeneratorRegistry::registerGenerator(ConfigWire wire,
                                                 ConfigParams params,
                                                 Factory factory) {
  params_[static_cast<uint8_t>(wire)] = params;
  factories_[static_cast<uint8_t>(wire)] = std::move(factory);
}

std::unique_ptr<AirTreeGenerator>
AirTreeGeneratorRegistry::create(ConfigWire wire) const {
  auto it = factories_.find(static_cast<uint8_t>(wire));
  if (it == factories_.end()) {
    return nullptr;
  }
  return it->second();
}

std::optional<ConfigParams>
AirTreeGeneratorRegistry::lookupParams(uint8_t wire) const {
  auto it = params_.find(wire);
  if (it == params_.end()) {
    return std::nullopt;
  }
  return it->second;
}


// Built-in registration
namespace {

template <typename G>
AirTreeGeneratorRegistry::Factory make_factory() {
  return [] { return std::make_unique<G>(); };
}

const bool kBuiltinsRegistered = [] {
  auto &reg = AirTreeGeneratorRegistry::instance();
  // wire, dims, bit_length, node_width, m_width, precision_bits
  reg.registerGenerator(ConfigWire::Config_1D_Tiny,
                        {ConfigWire::Config_1D_Tiny, 1, 13, 8, 4, 7},
                        make_factory<Generator1DxT>());
  reg.registerGenerator(ConfigWire::Config_1D_Fast,
                        {ConfigWire::Config_1D_Fast, 1, 16, 8, 4, 10},
                        make_factory<Generator1DxF>());
  reg.registerGenerator(ConfigWire::Config_1D_Precise,
                        {ConfigWire::Config_1D_Precise, 1, 20, 8, 5, 13},
                        make_factory<Generator1DxP>());
  reg.registerGenerator(ConfigWire::Config_2D_Fast,
                        {ConfigWire::Config_2D_Fast, 2, 10, 8, 3, 5},
                        make_factory<Generator2DxF>());
  reg.registerGenerator(ConfigWire::Config_2D_Precise,
                        {ConfigWire::Config_2D_Precise, 2, 12, 10, 4, 6},
                        make_factory<Generator2DxP>());
  reg.registerGenerator(ConfigWire::Config_3D_Fast,
                        {ConfigWire::Config_3D_Fast, 3, 10, 8, 3, 5},
                        make_factory<Generator3DxF>());
  reg.registerGenerator(ConfigWire::Config_3D_Precise,
                        {ConfigWire::Config_3D_Precise, 3, 12, 10, 4, 6},
                        make_factory<Generator3DxP>());
  reg.registerGenerator(ConfigWire::Config_4D_Fast,
                        {ConfigWire::Config_4D_Fast, 4, 10, 8, 3, 5},
                        make_factory<Generator4DxF>());
  reg.registerGenerator(ConfigWire::Config_4D_Precise,
                        {ConfigWire::Config_4D_Precise, 4, 12, 10, 4, 6},
                        make_factory<Generator4DxP>());
  return true;
}();

// Map (dims, ConfigType) → ConfigWire 
ConfigWire wireFromOptions(const AirTreeOptions &options) {
  switch (options.dimensions) {
  case 1:
    switch (options.type) {
    case ConfigType::XT: return ConfigWire::Config_1D_Tiny;
    case ConfigType::XF: return ConfigWire::Config_1D_Fast;
    case ConfigType::XP: return ConfigWire::Config_1D_Precise;
    default: throw std::invalid_argument("Unsupported configuration type for 1D");
    }
  case 2:
    switch (options.type) {
    case ConfigType::XF: return ConfigWire::Config_2D_Fast;
    case ConfigType::XP: return ConfigWire::Config_2D_Precise;
    default: throw std::invalid_argument("Unsupported configuration type for 2D");
    }
  case 3:
    switch (options.type) {
    case ConfigType::XF: return ConfigWire::Config_3D_Fast;
    case ConfigType::XP: return ConfigWire::Config_3D_Precise;
    default: throw std::invalid_argument("Unsupported configuration type for 3D");
    }
  case 4:
    switch (options.type) {
    case ConfigType::XF: return ConfigWire::Config_4D_Fast;
    case ConfigType::XP: return ConfigWire::Config_4D_Precise;
    default: throw std::invalid_argument("Unsupported configuration type for 4D");
    }
  default:
    throw std::invalid_argument("Unsupported dimension or configuration");
  }
}

} // namespace

std::unique_ptr<AirTreeGenerator>
AirTreeGeneratorFactory::create(const AirTreeOptions &options) {
  return create(wireFromOptions(options));
}

std::unique_ptr<AirTreeGenerator>
AirTreeGeneratorFactory::create(ConfigWire wire) {
  auto gen = AirTreeGeneratorRegistry::instance().create(wire);
  if (!gen) {
    throw std::invalid_argument(
        "AirTreeGeneratorFactory: no generator registered for wire 0x" +
        std::to_string(static_cast<unsigned>(wire)));
  }
  return gen;
}
