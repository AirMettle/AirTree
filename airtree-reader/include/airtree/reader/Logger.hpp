// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_READER_INCLUDE_AIRTREE_READER_LOGGER_HPP
#define AIRTREE_READER_INCLUDE_AIRTREE_READER_LOGGER_HPP


#include <airtree/util/logging/Logging.hpp> // IWYU pragma: keep
#include <spdlog/logger.h>                  // IWYU pragma: export
#include <spdlog/spdlog.h>                  // IWYU pragma: export
#include <memory>                           // for shared_ptr


namespace airtree::reader {

/**
 * Logger for the library
 */
std::shared_ptr<spdlog::logger> logger();

} // namespace airtree::reader


#endif // AIRTREE_READER_INCLUDE_AIRTREE_READER_LOGGER_HPP