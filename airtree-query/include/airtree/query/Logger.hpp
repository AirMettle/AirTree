// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_QUERY_INCLUDE_LOGGER_HPP
#define AIRTREE_QUERY_INCLUDE_LOGGER_HPP


#include <airtree/util/logging/Logging.hpp>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
#include <memory> // for shared_ptr


namespace airtree::query {

/**
 * Logger for the library
 */
std::shared_ptr<spdlog::logger> logger();

} // namespace airtree::query

#endif // AIRTREE_QUERY_INCLUDE_LOGGER_HPP
