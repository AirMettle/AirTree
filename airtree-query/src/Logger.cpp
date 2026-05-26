// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/query/Logger.hpp>
#include <airtree/util/logging/Logging.hpp>

using namespace airtree::util;

namespace airtree::query {

std::shared_ptr<spdlog::logger> logger() {
  static auto logger = logging::make_logger("airtree-query");
  return logger;
}

} // namespace airtree::query