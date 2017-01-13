#pragma once

#define ASIO_STANDALONE
#include "asio.hpp"

class visualizer_client
{
public:
    visualizer_client()
        : socket_(ioService_)
    {
        asio::ip::tcp::resolver resolver(ioService_);
        asio::ip::tcp::resolver::query query("localhost", "9000");
        auto endPointIt = resolver.resolve(query);
        asio::connect(socket_, endPointIt);
        send("BOUYA!");
    }

public:
    void send(const std::string &msg)
    {
        try
        {
            socket_.send(asio::buffer(msg));
        }
        catch (std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
    }

    void send(void *msg, size_t count)
    {

    }

private:
    asio::io_service        ioService_;
    asio::ip::tcp::socket   socket_;
};
