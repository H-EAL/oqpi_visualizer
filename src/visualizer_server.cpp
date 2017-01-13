#include "oqpi.hpp"
#include "visualizer_server.hpp"

int main()
{
    asio::io_service io_service;
    visualizer_server server(io_service);
    io_service.run();
}