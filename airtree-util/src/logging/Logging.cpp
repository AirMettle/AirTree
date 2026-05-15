// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/util/logging/Logging.hpp>

#include <spdlog/spdlog.h>                   // for SPDLOG_LOGGER_DEBUG
// #include <spdlog/sinks/ansicolor_sink-inl.h> // for ansicolor_sink::to_string_
#include <spdlog/sinks/stdout_color_sinks.h>
// #include <spdlog/sinks/ansicolor_sink.h>     // for ansicolor_stdout_sink
#include <spdlog/sinks/sink-inl.h>           // for sink::set_level
#include <spdlog/sinks/stdout_color_sinks.h> // for stdout_color_sink_mt
#include <spdlog/spdlog-inl.h>               // for flush_every, register_l...
#include <chrono>                            // for seconds
#include <utility>                           // for move

namespace airtree::util::logging {

std::shared_ptr<spdlog::logger>
make_logger(std::string name, spdlog::sinks_init_list sinks_init_list,
            spdlog::level::level_enum level) {

  auto logger =
      std::make_shared<spdlog::logger>(std::move(name), sinks_init_list);
  logger->set_level(level);

  spdlog::register_logger(logger);

  spdlog::flush_every(std::chrono::seconds(1));

  SPDLOG_LOGGER_DEBUG(logger, "Created logger '{}' (level: {})", logger->name(),
                      spdlog::level::to_string_view(logger->level()));

  return logger;
}

std::shared_ptr<spdlog::logger> make_logger(std::string name,
                                            spdlog::level::level_enum level) {

  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(level);
  console_sink->set_pattern(
      "[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [%t] [%@] [%!()]  %v");

  return make_logger(
      std::move(name), spdlog::sinks_init_list({console_sink}), level);
}

} // namespace airtree::util::logging