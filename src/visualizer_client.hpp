#pragma once

#include <mutex>

#define ASIO_STANDALONE
#include "asio.hpp"

#include "ring_buffer.hpp"


class visualizer_client
{
public:
    using buffer_type = std::vector<uint8_t>;

public:
    visualizer_client()
        : socket_(ioService_)
    {
        asio::ip::tcp::resolver resolver(ioService_);
        asio::ip::tcp::resolver::query query("localhost", "9000");
        auto endPointIt = resolver.resolve(query);
        asio::connect(socket_, endPointIt);
    }

public:
    template<typename ..._Args>
    void encodeAndSend(_Args &&...args)
    {
        buffer_type buffer(1024);
        size_t offset = sizeof(uint16_t); // First 2 bytes contain the size of the message
        encode(buffer, offset, std::forward<_Args>(args)...);
        buffer.resize(offset);
        const auto msgSize = uint16_t(buffer.size());
        memcpy(buffer.data(), &msgSize, sizeof(msgSize));
        send(buffer);
    }

    void send(const buffer_type &buffer)
    {
        try
        {
            //std::lock_guard<std::mutex> __l(mutex);
            //socket_.send(asio::buffer(buffer));
            asio::write(socket_, asio::buffer(buffer));
        }
        catch (std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
    }

private:
    template<typename T, typename ..._Args>
    void encode(buffer_type &buffer, size_t &offset, T &&t, _Args &&...args)
    {
        encodeValue(buffer, offset, std::forward<T>(t));
        encode(buffer, offset, std::forward<_Args>(args)...);
    }

    void encode(buffer_type &buffer, size_t &offset)
    {}

    template<typename T>
    void encodeValue(buffer_type &buffer, size_t &offset, T &&t)
    {
        memcpy(buffer.data() + offset, &t, sizeof(T));
        offset += sizeof(T);
    }

    void encodeValue(buffer_type &buffer, size_t &offset, const std::string &s)
    {
        encodeValue(buffer, offset, s.size());
        memcpy(buffer.data() + offset, s.data(), s.size());
        offset += s.size();
    }

private:
    asio::io_service        ioService_;
    asio::ip::tcp::socket   socket_;
    std::mutex              mutex;
    //static thread_local ring_buffer buffer_;
    oqpi::thread_interface<>  senderThread_;
};
