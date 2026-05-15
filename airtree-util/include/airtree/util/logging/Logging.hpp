// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_UTIL_LOGGING_LOGGING_HPP
#define AIRTREE_UTIL_LOGGING_LOGGING_HPP


#include <airtree/util/logging/LoggingConfig.hpp>
#include <spdlog/common.h> // for level_enum, sinks_init_list
#include <spdlog/logger.h> // IWYU pragma: export
#include <spdlog/spdlog.h> // IWYU pragma: export

// TODO : ADD namespace

namespace airtree::util::logging {

std::shared_ptr<spdlog::logger>
make_logger(std::string name, spdlog::sinks_init_list sinks_init_list,
            spdlog::level::level_enum level =
                static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL));

std::shared_ptr<spdlog::logger>
make_logger(std::string name,
            spdlog::level::level_enum level =
                static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL));

} // namespace airtree::util::logging


#endif // AIRTREE_UTIL_LOGGING_LOGGING_HPP