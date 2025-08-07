#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t numThreads) : stop_(false) {
    for (size_t i = 0; i < numThreads; ++i)
        workers_.emplace_back([this] { this->workerLoop(); });
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::shutdown() {
    stop_ = true;
    cv_.notify_all();
    for (auto& th : workers_) {
        if (th.joinable()) th.join();
    }
    workers_.clear();
}

void ThreadPool::workerLoop() {
    while (!stop_) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMtx_);
            cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
            if (stop_ && tasks_.empty()) return;
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        task();
    }
}
