# HTTP-TCP server
This is a simple TCP server built in C++.

Credit: [Building Http server from scratch](https://osasazamegbe.medium.com/showing-building-an-http-server-from-scratch-in-c-2da7c0db6cb7)

The above article provides an amazing resource to build a basic TCP server leveraging linux APIs.

Extending this project to build the http request/response pipeline over the basic project.

## Build steps
in cpp_webserver folder run the following commands
```cmake
cmake -S . -B build
cmake --build build
```
Run the following for configuration:

```cmake
./build/server --help
```