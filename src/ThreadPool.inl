#pragma once
#include <stdexcept>

template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
    using return_type = decltype(f(args...));
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    std::future<return_type> res = task->get_future();
    {
        std::lock_guard<std::mutex> lock(queueMtx_);
        if (stop_)
            throw std::runtime_error("ThreadPool is stopped");
        tasks_.emplace([task]() { (*task)(); });
    }
    cv_.notify_one();
    return res;
}
