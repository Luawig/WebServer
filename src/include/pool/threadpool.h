//
// Created by leon on 5/2/23.
//

#ifndef WEBSERVER_THREADPOOL_H
#define WEBSERVER_THREADPOOL_H

#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <iostream>
#include <condition_variable>

class ThreadPool {
  public:
    explicit ThreadPool(unsigned int thread_num);

    ~ThreadPool();

    void addTask(std::function<void()> task);

    [[maybe_unused]] [[nodiscard]] bool is_running() const { return is_running_; }

    [[maybe_unused]] [[nodiscard]] unsigned int threadNum() const { return threadNum_; }

    [[maybe_unused]] void addThread(unsigned int thread_num);

    void stop();

  private:
    void run_();

    std::vector<std::shared_ptr<std::thread>> threads_;

    std::queue<std::function<void()>> tasks_;

    std::mutex threadsMux_;
    std::mutex tasksMux_;

    bool is_running_{};

    unsigned int threadNum_{};

    std::condition_variable cv_;
};


#endif //WEBSERVER_THREADPOOL_H
