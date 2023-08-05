//
// Created by leon on 5/2/23.
//

#include "pool/threadpool.h"

ThreadPool::ThreadPool(unsigned int thread_count) : is_running_(true), threadCount_(thread_count) {
    if (thread_count <= 0) {
        throw std::invalid_argument("Thread count must be positive.");
    }
    for (int i = 0; i < thread_count; ++i) {
        auto thread = std::make_shared<std::thread>(&ThreadPool::run_, this);
        threads_.push_back(thread);
    }
}

ThreadPool::~ThreadPool() {
    if (is_running_)
        this->stop();
}

void ThreadPool::run_() {
    while (is_running_) {
        std::unique_lock<std::mutex> lock(this->tasksMux_);
        if (tasks_.empty()) this->cv_.wait(lock);
        if (tasks_.empty()) continue;
        auto task = tasks_.front();
        tasks_.pop();
        task();
    }
}

void ThreadPool::addTask(std::function<void()>task) {
    {
        std::unique_lock<std::mutex> lock(this->tasksMux_);
        tasks_.emplace(std::forward<std::function<void()>>(task));
    }
    this->cv_.notify_one();
}

void ThreadPool::addThread(unsigned int thread_count) {
    if (thread_count <= 0) {
        throw std::invalid_argument("Thread count must be positive.");
    }
    this->threadCount_ += thread_count;
    std::unique_lock<std::mutex> lock(this->threadsMux_);
    for (int i = 0; i < thread_count; ++i) {
        auto thread = std::make_shared<std::thread>(&ThreadPool::run_, this);
        threads_.push_back(thread);
    }
}

void ThreadPool::stop() {
    this->is_running_ = false;
    this->cv_.notify_all();
    {
        std::unique_lock<std::mutex> lock(threadsMux_);
        for (auto &thread: threads_) {
            thread->join();
        }
    }
}
