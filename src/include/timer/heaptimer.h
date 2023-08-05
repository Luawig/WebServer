//
// Created by leon on 5/2/23.
//

#ifndef WEBSERVER_HEAPTIMER_H
#define WEBSERVER_HEAPTIMER_H

#include <queue>
#include <unordered_map>
#include "timer.h"

class HeapTimer {
public:
    HeapTimer();

    ~HeapTimer();

    void adjust(int id, int newExpires);

    void add(int id, int timeOut, const TimeoutCallBack &cb);

    void clear();

    int getNextTick();

private:
    std::priority_queue<Timer> heap_;
    std::unordered_map<int, TimeStamp> map_;
};

#endif //WEBSERVER_HEAPTIMER_H
