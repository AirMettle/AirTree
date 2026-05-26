// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/cli/Logger.hpp>
#include <airtree/util/logging/Logging.hpp>

using namespace airtree::util;

namespace airtree::cli {

std::shared_ptr<spdlog::logger> logger() {
  static auto logger = logging::make_logger("airtree");
  return logger;
}

} // namespace airtree::cli
