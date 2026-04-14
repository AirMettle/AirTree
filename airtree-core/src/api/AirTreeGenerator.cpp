#include <airtree/core/api/AirTreeGenerator.hpp>
#include <airtree/core/AirTreeCore_internal.hpp>

using namespace airtree::core::api;
using namespace airtree::core::schema::trie1d;
using namespace airtree::core::schema::trie2d;
using namespace airtree::core::schema::trie3d;
using namespace airtree::core::schema::trie4d;

std::unique_ptr<AirTreeGenerator>
AirTreeGeneratorFactory::create(const AirTreeOptions &options) {
  if (options.dimensions == 1) {
    switch (options.type) {
    case ConfigType::XF:
      return std::make_unique<Generator1DxF>();
    case ConfigType::XT:
      return std::make_unique<Generator1DxT>();
    case ConfigType::XP:
      return std::make_unique<Generator1DxP>();
    case ConfigType::XNUM:
    default:
      throw std::invalid_argument("Unsupported configuration type for 1D");
    }
  } else if (options.dimensions == 2) {
    switch (options.type) {
    case ConfigType::XF:
      return std::make_unique<Generator2DxF>();
    case ConfigType::XP:
      return std::make_unique<Generator2DxP>();
    case ConfigType::XNUM:
    default:
      throw std::invalid_argument("Unsupported configuration type for 2D");
    }
  } else if (options.dimensions == 3) {
    switch (options.type) {
    case ConfigType::XF:
      return std::make_unique<Generator3DxF>();
    case ConfigType::XP:
      return std::make_unique<Generator3DxP>();
    case ConfigType::XNUM:
    default:
      throw std::invalid_argument("Unsupported configuration type for 3D");
    }
  } else if (options.dimensions == 4) {
    switch (options.type) {
    case ConfigType::XF:
      return std::make_unique<Generator4DxF>();
    case ConfigType::XP:
      return std::make_unique<Generator4DxP>();
    case ConfigType::XNUM:
    default:
      throw std::invalid_argument("Unsupported configuration type for 4D");
    }
  }

  throw std::invalid_argument("Unsupported dimension or configuration");
}
