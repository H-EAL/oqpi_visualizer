#pragma once

#include <thread>

#define ASIO_STANDALONE
#include "asio.hpp"

#include "timer_contexts.hpp"

using buffer_type = std::vector<uint8_t>;

//--------------------------------------------------------------------------------------------------
class telemetry
{
public:
    void process(const buffer_type &buffer)
    {
        uint16_t msgSize = 0;
        size_t offset = 0;
        task_info ti;
        std::string name;

        decode(buffer, offset, msgSize, ti, name);
        oqpi_check(msgSize == buffer.size());

        taskNames_[ti.uid] = name;
        tasks_[ti.uid] = ti;
        printDuration(ti, ti.uid);
    }

private:
    void printDuration(const task_info &ti, oqpi::task_uid uid)
    {
        std::cout
            << getFullName(ti)
            << " ended after "
            << duration(ti.startedAt, ti.stoppedAt)
            << "ms"
            << std::endl;
    }

    std::string getFullName(const task_info &ti)
    {
        std::string fullName;
        if (ti.groupUID != oqpi::invalid_task_uid)
        {
            auto it = tasks_.find(ti.groupUID);
            if (it != tasks_.end())
            {
                fullName = getFullName(tasks_[ti.groupUID]) + "/";
            }
        }
        return fullName + taskNames_[ti.uid];
    }

    template<typename T, typename ..._Args>
    void decode(const buffer_type &buffer, size_t &offset, T &t, _Args &...args)
    {
        decodeVar(buffer, offset, t);
        decode(buffer, offset, args...);
    }

    void decode(const buffer_type &buffer, size_t &offset)
    {}

    template<typename T>
    void decodeVar(const buffer_type &buffer, size_t &offset, T &t)
    {
        memcpy(&t, buffer.data() + offset, sizeof(T));
        offset += sizeof(T);
    }

    void decodeVar(const buffer_type &buffer, size_t &offset, std::string &s)
    {
        size_t length = 0;
        decodeVar(buffer, offset, length);
        s.resize(length);
        memcpy((void*)s.data(), buffer.data() + offset, length);
        offset += length;
    }

private:
    std::unordered_map<oqpi::task_uid, task_info> tasks_;
    std::unordered_map<oqpi::task_uid, std::string> taskNames_;
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
                telemetry t;
                try
                {
                    for (;;)
                    {
                        buffer_type buffer(2);
                        asio::error_code error;
                        size_t length = sock.receive(asio::buffer(buffer));
                        if (error == asio::error::eof)
                            break; // Connection closed cleanly by peer.
                        else if (error)
                            throw asio::system_error(error); // Some other error.

                        uint16_t bufferSize = *((uint16_t*)buffer.data());
                        buffer.resize(bufferSize);
                        size_t offset = 2;
                        length = 0;
                        while (length != bufferSize - 2)
                        {
                            length += sock.receive(asio::buffer(buffer.data() + offset, bufferSize - offset));
                            if (error == asio::error::eof)
                                break; // Connection closed cleanly by peer.
                            else if (error)
                                throw asio::system_error(error); // Some other error.
                        }
                        oqpi_check(length == bufferSize - 2);
                        buffer.resize(bufferSize);
                        t.process(buffer);
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
    asio::ip::tcp::acceptor acceptor_;
};
//--------------------------------------------------------------------------------------------------
