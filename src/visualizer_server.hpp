#pragma once

#include <thread>

#define ASIO_STANDALONE
#include "asio.hpp"


//--------------------------------------------------------------------------------------------------
class tcp_connection
    : public std::enable_shared_from_this<tcp_connection>
{
public:
    using pointer = std::shared_ptr<tcp_connection>;

public:
    static pointer create(asio::io_service &ioService)
    {
        return pointer(new tcp_connection(ioService));
    }

public:
    asio::ip::tcp::socket& socket()
    {
        return socket_;
    }

    void start()
    {
        std::vector<uint8_t> v;
        while (socket_.receive(asio::buffer(v)))
        {
            std::cout << v.size() << std::endl;
        }
    }

private:
    tcp_connection(asio::io_service &ioService)
        : socket_(ioService)
    {}

private:
    asio::ip::tcp::socket socket_;
};
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
class visualizer_server
{
public:
    visualizer_server(asio::io_service &ioService)
        : acceptor_(ioService, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 9000))
    {
        for (;;)
        {
            asio::ip::tcp::socket sock(ioService);
            acceptor_.accept(sock);
            std::thread([](asio::ip::tcp::socket sock)
            {
                try
                {
                    for (;;)
                    {
                        static const auto max_length = 1024;
                        char data[max_length];

                        asio::error_code error;
                        size_t length = sock.read_some(asio::buffer(data), error);
                        if (error == asio::error::eof)
                            break; // Connection closed cleanly by peer.
                        else if (error)
                            throw asio::system_error(error); // Some other error.

                        std::cout << std::string(&data[0], length) << std::endl;
                    }
                }
                catch (std::exception& e)
                {
                    std::cerr << "Exception in thread: " << e.what() << "\n";
                }
            }, std::move(sock)).detach();
        }
    }

private:
    void startAccept()
    {
        tcp_connection::pointer newConnection = tcp_connection::create(acceptor_.get_io_service());
        acceptor_.async_accept(newConnection->socket(), std::bind(&visualizer_server::handleAccept, this, newConnection, std::placeholders::_1));
    }

    void handleAccept(tcp_connection::pointer newConnection, const asio::error_code &error)
    {
        if (!error)
        {
            std::cout << "New Connection!" << std::endl;
            newConnection->start();
        }

        startAccept();
    }

private:
    asio::ip::tcp::acceptor acceptor_;
};
//--------------------------------------------------------------------------------------------------
