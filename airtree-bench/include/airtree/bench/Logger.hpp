#ifndef AIRTREE_BENCH_INCLUDE_AIRTREE_BENCH_LOGGER_HPP
#define AIRTREE_BENCH_INCLUDE_AIRTREE_BENCH_LOGGER_HPP


#include <airtree/util/logging/Logging.hpp> // IWYU pragma: keep
#include <spdlog/logger.h>                  // IWYU pragma: export
#include <spdlog/spdlog.h>                  // IWYU pragma: export
#include <memory>                           // for shared_ptr


namespace airtree::bench {

/**
 * Logger for the library
 */
std::shared_ptr<spdlog::logger> logger();

} // namespace airtree::bench


#endif // AIRTREE_BENCH_INCLUDE_AIRTREE_BENCH_LOGGER_HPP
