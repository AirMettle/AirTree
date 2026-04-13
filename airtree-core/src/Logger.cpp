#include <airtree/core/Logger.hpp>
#include <airtree/util/logging/Logging.hpp>

using namespace airtree::util;

namespace airtree::core {

std::shared_ptr<spdlog::logger> logger() {
  static auto logger = logging::make_logger("airtree-core");
  return logger;
}

} // namespace airtree::core