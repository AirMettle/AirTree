#ifndef AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_LOGGER_HPP
#define AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_LOGGER_HPP


#include <airtree/util/logging/Logging.hpp>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
#include <memory> // for shared_ptr


namespace airtree::merge {

/**
 * Logger for the library
 */
std::shared_ptr<spdlog::logger> logger();

} // namespace airtree::merge

#endif // AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_LOGGER_HPP
