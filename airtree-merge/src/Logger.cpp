#include <airtree/merge/Logger.hpp>
#include <airtree/util/logging/Logging.hpp>

using namespace airtree::util;

namespace airtree::merge {

std::shared_ptr<spdlog::logger> logger() {
  static auto logger = logging::make_logger("airtree-merge");
  return logger;
}

} // namespace airtree::merge
