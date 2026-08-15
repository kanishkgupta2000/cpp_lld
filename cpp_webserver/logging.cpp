#include "logging.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace {

spdlog::level::level_enum to_spdlog_level(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Trace:
        return spdlog::level::trace;
    case LogLevel::Debug:
        return spdlog::level::debug;
    case LogLevel::Info:
        return spdlog::level::info;
    case LogLevel::Warn:
        return spdlog::level::warn;
    case LogLevel::Error:
        return spdlog::level::err;
    case LogLevel::Critical:
        return spdlog::level::critical;
    case LogLevel::Off:
        return spdlog::level::off;
    }
    return spdlog::level::info;
}

} // namespace

void init_logging(const ServerConfig &config)
{
    auto logger = spdlog::stdout_color_mt("server");
    logger->set_level(to_spdlog_level(config.log_level));
    spdlog::set_default_logger(logger);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
}
