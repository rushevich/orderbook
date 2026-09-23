#pragma once
#include <pthread.h>
#include <sched.h>
#include <system_error>
#include <unistd.h>
namespace rushevich::system {
inline void pinThread(int cpu) {
    if (cpu < 0) {
        return;
    }
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == -1) {
        throw std::system_error(errno, std::system_category(), "Failed to set thread affinity");
    }
}

} // namespace rushevich::system
