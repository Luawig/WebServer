//
// Created by leon on 5/3/23.
//

#ifndef WEBSERVER_EPOLLER_H
#define WEBSERVER_EPOLLER_H

#include <sys/epoll.h>
#include <unistd.h>
#include <vector>

class Epoller {
  public:
    explicit Epoller(int maxEvent = 1024) : epollFd_(epoll_create(512)), events_(maxEvent) {}

    ~Epoller() { close(epollFd_); }

    [[nodiscard]] bool AddFd(int fd, uint32_t events) const;

    [[nodiscard]] bool ModFd(int fd, uint32_t events) const;

    [[nodiscard]] bool DelFd(int fd) const;

    int Wait(int timeoutMs = -1);

    [[nodiscard]] int GetEventFd(size_t i) const {
        return events_[i].data.fd;
    }

    [[nodiscard]] uint32_t GetEvents(size_t i) const {
        return events_[i].events;
    }

  private:
    int epollFd_;

    std::vector<struct epoll_event> events_;
};


#endif //WEBSERVER_EPOLLER_H
