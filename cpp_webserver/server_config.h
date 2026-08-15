#ifndef INCLUDED_SERVER_CONFIG
#define INCLUDED_SERVER_CONFIG

#include <string>

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Critical,
    Off,
};

struct ServerConfig {
    std::string host = "0.0.0.0";
    int port = 8080;
    bool run_sequential = false;
    LogLevel log_level = LogLevel::Info;

    static ServerConfig parse(int argc, char *argv[]);
    static void print_usage(const char *program_name);
};

#endif
