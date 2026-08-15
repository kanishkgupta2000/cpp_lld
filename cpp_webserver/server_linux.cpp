#include "http_tcpServer_linux.h"
#include "logging.h"
#include "server_config.h"
#include<spdlog/spdlog.h>

int main(int argc, char *argv[])
{
    const ServerConfig config = ServerConfig::parse(argc, argv);
    init_logging(config);
    http::TcpServer server(config);
    server.startListen();
    return 0;
}
