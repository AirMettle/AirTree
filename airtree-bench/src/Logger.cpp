#include <airtree/bench/Logger.hpp>
#include <airtree/util/logging/Logging.hpp>

using namespace airtree::util;

namespace airtree::bench {

std::shared_ptr<spdlog::logger> logger() {
  static auto logger = logging::make_logger("airtree-bench");
  return logger;
}

} // namespace airtree::bench
