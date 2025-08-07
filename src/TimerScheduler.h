#pragma once
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <memory>

class ThreadPool;

struct ScheduledTask {
    size_t id;
    std::chrono::steady_clock::time_point nextRunTime;
    uint32_t intervalMs;
    std::function<void()> task;
};

class TimerScheduler {
public:
    using TimerHandle = size_t;

    explicit TimerScheduler(std::shared_ptr<ThreadPool> pool);
    ~TimerScheduler();
    TimerHandle scheduleEvery(uint32_t intervalMs, std::function<void()> task);
    void cancel(TimerHandle handle);
    void start();
    void stop();

private:
    std::mutex mtx_;
    std::condition_variable cv_;
    std::thread timerThread_;
    std::atomic<bool> running_{false};
    std::shared_ptr<ThreadPool> pool_;
    std::atomic<size_t> nextId_{1};
    std::unordered_map<TimerHandle, ScheduledTask> tasks_;
    void run();
};

