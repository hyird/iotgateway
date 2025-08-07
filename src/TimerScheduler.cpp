#include "TimerScheduler.h"
#include "ThreadPool.h"

TimerScheduler::TimerScheduler(std::shared_ptr<ThreadPool> pool)
    : running_(false), pool_(std::move(pool)) {}

TimerScheduler::~TimerScheduler() {
    stop();
}

TimerScheduler::TimerHandle TimerScheduler::scheduleEvery(uint32_t intervalMs, std::function<void()> task) {
    std::lock_guard<std::mutex> lock(mtx_);
    const TimerHandle id = nextId_++;
    ScheduledTask st;
    st.id = id;
    st.intervalMs = intervalMs;
    st.task = std::move(task);
    st.nextRunTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(intervalMs);
    tasks_[id] = st;
    cv_.notify_one();
    return id;
}

void TimerScheduler::cancel(const TimerHandle handle) {
    std::lock_guard<std::mutex> lock(mtx_);
    tasks_.erase(handle);
    cv_.notify_one();
}

void TimerScheduler::start() {
    if (running_) return;
    running_ = true;
    timerThread_ = std::thread(&TimerScheduler::run, this);
}

void TimerScheduler::stop() {
    running_ = false;
    cv_.notify_all();
    if (timerThread_.joinable())
        timerThread_.join();
}

void TimerScheduler::run() {
    while (running_) {
        std::unique_lock<std::mutex> lock(mtx_);
        if (tasks_.empty()) {
            cv_.wait_for(lock, std::chrono::milliseconds(50));
            continue;
        }
        // 找到最早到期任务
        const auto nextIt = std::min_element(
            tasks_.begin(), tasks_.end(),
            [](const auto& a, const auto& b) { return a.second.nextRunTime < b.second.nextRunTime; }
        );
        if (auto now = std::chrono::steady_clock::now(); now >= nextIt->second.nextRunTime) {
            ScheduledTask task = nextIt->second;
            lock.unlock();
            pool_->enqueue(task.task);
            lock.lock();
            if (tasks_.find(task.id) != tasks_.end()) {
                tasks_[task.id].nextRunTime = now + std::chrono::milliseconds(task.intervalMs);
            }
            lock.unlock();
        } else {
            cv_.wait_until(lock, nextIt->second.nextRunTime);
        }
    }
}

