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

int32_t query_performance_counter()
{
    first_measure();
    const auto t = query_performance_counter_aux();
    return int32_t(t - first_measure());
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


class timing_registry
{
public:
    enum opcode : uint32_t
    {
        register_task,
        add_to_group,
        unregister_task,
        start_task,
        end_task,

        count
    };

    timing_registry()
    {}

    static timing_registry& get()
    {
        static timing_registry instance;
        return instance;
    }

    void registerTask(oqpi::task_uid uid, const std::string &name)
    {
        auto t = query_performance_counter();

        std::string msg = "Task #";
        msg += std::to_string(uid);
        msg += " named ";
        msg += name;
        msg += " created @t=";
        msg += std::to_string(t);

        client_.send(msg);
    }

    void unregisterTask(oqpi::task_uid uid)
    {
        auto t = query_performance_counter();

        std::string msg = "Task #";
        msg += std::to_string(uid);
        msg += " deleted @t=";
        msg += std::to_string(t);

        client_.send(msg);
    }

    void startTask(oqpi::task_uid uid)
    {
        auto t = query_performance_counter();
        auto c = oqpi::this_thread::get_current_core();

        std::string msg = "Task #";
        msg += std::to_string(uid);
        msg += " started on core ";
        msg += std::to_string(c);
        msg += " @t=";
        msg += std::to_string(t);

        client_.send(msg);
    }

    void endTask(oqpi::task_uid uid)
    {
        auto t = query_performance_counter();
        auto c = oqpi::this_thread::get_current_core();

        std::string msg = "Task #";
        msg += std::to_string(uid);
        msg += " ended on core ";
        msg += std::to_string(c);
        msg += " @t=";
        msg += std::to_string(t);

        client_.send(msg);
    }

    void addToGroup(oqpi::task_uid guid, oqpi::task_handle hTask)
    {
        auto t = query_performance_counter();

        std::string msg = "Task #";
        msg += std::to_string(hTask.getUID());
        msg += " added to group #";
        msg += std::to_string(guid);
        msg += " @t=";
        msg += std::to_string(t);

        client_.send(msg);
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
        timing_registry::get().registerTask(pOwner->getUID(), name);
    }

    ~timer_task_context()
    {
        timing_registry::get().unregisterTask(this->owner()->getUID());
    }

    inline void onPreExecute()
    {
        timing_registry::get().startTask(this->owner()->getUID());
    }

    inline void onPostExecute()
    {
        timing_registry::get().endTask(this->owner()->getUID());
    }

private:
};


class timer_group_context
    : public oqpi::group_context_base
{
public:
    timer_group_context(oqpi::task_group_base *pOwner, const std::string &name)
        : oqpi::group_context_base(pOwner, name)
    {
        timing_registry::get().registerTask(pOwner->getUID(), name);
    }

    ~timer_group_context()
    {
        timing_registry::get().unregisterTask(this->owner()->getUID());
    }

    inline void onTaskAdded(const oqpi::task_handle &hTask)
    {
        timing_registry::get().addToGroup(this->owner()->getUID(), hTask);
    }

    inline void onPreExecute()
    {
        timing_registry::get().startTask(this->owner()->getUID());
    }

    inline void onPostExecute()
    {
        timing_registry::get().endTask(this->owner()->getUID());
    }
};


namespace this_task {

}
