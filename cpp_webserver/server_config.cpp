#include "server_config.h"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

bool parse_log_level(std::string_view value, LogLevel &out)
{
    if (value == "trace")
    {
        out = LogLevel::Trace;
        return true;
    }
    if (value == "debug")
    {
        out = LogLevel::Debug;
        return true;
    }
    if (value == "info")
    {
        out = LogLevel::Info;
        return true;
    }
    if (value == "warn" || value == "warning")
    {
        out = LogLevel::Warn;
        return true;
    }
    if (value == "error")
    {
        out = LogLevel::Error;
        return true;
    }
    if (value == "critical")
    {
        out = LogLevel::Critical;
        return true;
    }
    if (value == "off")
    {
        out = LogLevel::Off;
        return true;
    }
    return false;
}

bool requires_value(int index, int argc, const char *flag)
{
    if (index + 1 >= argc)
    {
        std::cerr << "error: " << flag << " requires a value\n";
        return false;
    }
    return true;
}

} // namespace

void ServerConfig::print_usage(const char *program_name)
{
    std::cout
        << "Usage: " << program_name << " [options]\n\n"
        << "Options:\n"
        << "  --host ADDR           Bind address (default: 0.0.0.0)\n"
        << "  --port PORT           Listen port (default: 8080)\n"
        << "  --sequential          Handle one request at a time (no thread pool)\n"
        << "  --log-level LEVEL     trace|debug|info|warn|error|critical|off (default: info)\n"
        << "  --silent              Disable logging (same as --log-level off)\n"
        << "  -h, --help            Show this help message\n";
}

ServerConfig ServerConfig::parse(int argc, char *argv[])
{
    ServerConfig config;

    for (int i = 1; i < argc; ++i)
    {
        const std::string_view arg(argv[i]);

        if (arg == "-h" || arg == "--help")
        {
            print_usage(argv[0]);
            std::exit(0);
        }

        if (arg == "--sequential")
        {
            config.run_sequential = true;
            continue;
        }

        if (arg == "--silent")
        {
            config.log_level = LogLevel::Off;
            continue;
        }

        if (arg == "--host")
        {
            if (!requires_value(i, argc, "--host"))
            {
                print_usage(argv[0]);
                std::exit(1);
            }
            config.host = argv[++i];
            continue;
        }

        if (arg == "--port")
        {
            if (!requires_value(i, argc, "--port"))
            {
                print_usage(argv[0]);
                std::exit(1);
            }
            config.port = std::stoi(argv[++i]);
            continue;
        }

        if (arg == "--log-level")
        {
            if (!requires_value(i, argc, "--log-level"))
            {
                print_usage(argv[0]);
                std::exit(1);
            }
            if (!parse_log_level(argv[++i], config.log_level))
            {
                std::cerr << "error: invalid log level '" << argv[i] << "'\n";
                print_usage(argv[0]);
                std::exit(1);
            }
            continue;
        }

        std::cerr << "error: unknown argument '" << arg << "'\n";
        print_usage(argv[0]);
        std::exit(1);
    }

    return config;
}
