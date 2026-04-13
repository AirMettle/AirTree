#include <airtree/reader/Logger.hpp>
#include <airtree/util/logging/Logging.hpp>

using namespace airtree::util;

namespace airtree::reader {

std::shared_ptr<spdlog::logger> logger() {
  static auto logger = logging::make_logger("airtree-reader");
  return logger;
}

} // namespace airtree::reader