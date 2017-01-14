#pragma once

#include <chrono>
#include <unordered_map>
#include "oqpi.hpp"
#include "visualizer_client.hpp"


int64_t query_performance_counter_aux()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}

int64_t first_measure()
{
    static const auto firstMeasure = query_performance_counter_aux();
    return firstMeasure;
}

uint32_t query_performance_counter()
{
    first_measure();
    const auto t = query_performance_counter_aux();
    return uint32_t(t - first_measure());
}

int64_t query_performance_frequency()
{
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    return li.QuadPart;
}

double duration(int64_t s, int64_t e)
{
    static const auto F = query_performance_frequency();
    auto dt = e - s;
    return (dt / (F*1.0)) * 1000.0;
}

enum opcode : uint8_t
{
    register_task,
    unregister_task,
    add_to_group,
    start_task,
    end_task,

    count
};

struct task_info
{
    using thread_id = oqpi::thread_interface<>::id;

    oqpi::task_uid  uid             = oqpi::invalid_task_uid;
    oqpi::task_uid  groupUID        = oqpi::invalid_task_uid;
    uint32_t        startedAt       = 0;
    uint32_t        stoppedAt       = 0;
    thread_id       startedOnThread = 0;
    thread_id       stoppedOnThread = 0;
    uint8_t         startedOnCore   = 0xFF;
    uint8_t         stoppedOnCore   = 0xFF;
};

class timing_registry
{
public:

    timing_registry()
    {}

    static timing_registry& get()
    {
        static timing_registry instance;
        return instance;
    }

    void send(const task_info &ti, const std::string &name)
    {
        client_.encodeAndSend(ti, name);
    }

private:
    visualizer_client client_;
};

class timer_task_context
    : public oqpi::task_context_base
{
public:
    timer_task_context(oqpi::task_base *pOwner, const std::string &name)
        : oqpi::task_context_base(pOwner, name)
    {
        ti_.uid = pOwner->getUID();
        name_   = name;
    }

    ~timer_task_context()
    {
        timing_registry::get().send(ti_, name_);
    }

    inline void onAddedToGroup(const oqpi::task_group_sptr &spParentGroup)
    {
        ti_.groupUID = spParentGroup->getUID();
    }

    inline void onPreExecute()
    {
        ti_.startedOnCore   = oqpi::this_thread::get_current_core();
        ti_.startedOnThread = oqpi::this_thread::get_id();
        ti_.startedAt       = query_performance_counter();
    }

    inline void onPostExecute()
    {
        ti_.stoppedAt       = query_performance_counter();
        ti_.startedOnCore   = oqpi::this_thread::get_current_core();
        ti_.startedOnThread = oqpi::this_thread::get_id();
    }

    task_info   ti_;
    std::string name_;
};


class timer_group_context
    : public oqpi::group_context_base
{
public:
    timer_group_context(oqpi::task_group_base *pOwner, const std::string &name)
        : oqpi::group_context_base(pOwner, name)
    {
        ti_.uid = pOwner->getUID();
        name_ = name;
    }

    ~timer_group_context()
    {
    }

    inline void onAddedToGroup(const oqpi::task_group_sptr &spParentGroup)
    {
        ti_.groupUID = spParentGroup->getUID();
    }

    inline void onPreExecute()
    {
        ti_.startedOnCore = oqpi::this_thread::get_current_core();
        ti_.startedOnThread = oqpi::this_thread::get_id();
        ti_.startedAt = query_performance_counter();
    }

    inline void onPostExecute()
    {
        ti_.stoppedAt = query_performance_counter();
        ti_.startedOnCore = oqpi::this_thread::get_current_core();
        ti_.startedOnThread = oqpi::this_thread::get_id();
        timing_registry::get().send(ti_, name_);
    }

    task_info   ti_;
    std::string name_;
};
