//
// Created by leon on 5/2/23.
//

#ifndef WEBSERVER_TIMER_H
#define WEBSERVER_TIMER_H

#include <chrono>
#include <functional>
#include <utility>

typedef std::function<void()> TimeoutCallBackType;
typedef std::chrono::high_resolution_clock Clock;
typedef Clock::time_point TimeStamp;

class Timer {
public:
    int id;
    TimeStamp expires;
    TimeoutCallBackType cb;

    Timer(int id, TimeStamp expires, TimeoutCallBackType cb) : id(id), expires(expires), cb(std::move(cb)) {};

    bool operator<(const Timer &rhs) const {
        return expires < rhs.expires;
    }
};


#endif //WEBSERVER_TIMER_H
