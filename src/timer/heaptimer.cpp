//
// Created by leon on 5/2/23.
//

#include "timer/heaptimer.h"

HeapTimer::HeapTimer() = default;

HeapTimer::~HeapTimer() {
    clear();
}

void HeapTimer::adjust(int id, int newExpires) {
    map_[id] = static_cast<std::chrono::milliseconds>(newExpires) + Clock::now();
}

void HeapTimer::add(int id, int timeout, const TimeoutCallBack &cb) {
    heap_.emplace(id, static_cast<std::chrono::milliseconds>(timeout) + Clock::now(), cb);
}

void HeapTimer::clear() {
    while (!heap_.empty()) heap_.pop();
    map_.clear();
}

int HeapTimer::getNextTick() {
    while (!heap_.empty() &&
           std::chrono::duration_cast<std::chrono::milliseconds>(heap_.top().expires - Clock::now()).count() <= 0) {
        heap_.top().cb();
        heap_.pop();
    }
    if (heap_.empty()) return -1;
    return std::chrono::duration_cast<std::chrono::milliseconds>(heap_.top().expires - Clock::now()).count();
}
