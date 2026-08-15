Command to run the project

cpp_webserver % g++ -std=c++20 \
  -I/opt/homebrew/opt/boost/include \
  -I/opt/homebrew/opt/spdlog/include \
  -I/opt/homebrew/opt/fmt/include \
  server_linux.cpp http_tcpServer_linux.cpp rest_pipeline.cpp logging.cpp server_config.cpp  \
  -L/opt/homebrew/opt/fmt/lib \
  -lfmt \
  -o server