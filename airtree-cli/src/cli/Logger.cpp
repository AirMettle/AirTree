#include <airtree/cli/Logger.hpp>
#include <airtree/util/logging/Logging.hpp>

using namespace airtree::util;

namespace airtree::cli {

std::shared_ptr<spdlog::logger> logger() {
  static auto logger = logging::make_logger("airtree-cli");
  return logger;
}

} // namespace airtree::cli
