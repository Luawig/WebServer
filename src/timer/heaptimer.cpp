//
// Created by leon on 5/2/23.
//

#include "timer/heaptimer.h"

void HeapTimer::adjust(int id, int newExpires) {
    std::vector<Timer> tmp;
    while (!heap_.empty()) {
        Timer timer = heap_.top();
        heap_.pop();
        if (timer.id == id) {
            timer.expires = static_cast<std::chrono::milliseconds>(newExpires) + Clock::now();
            tmp.push_back(timer);
            break;
        } else {
            tmp.push_back(timer);
        }
    }
    for (auto &timer : tmp) {
        heap_.push(timer);
    }
}

void HeapTimer::add(int id, int timeout, const TimeoutCallBackType &cb) {
    heap_.emplace(id, static_cast<std::chrono::milliseconds>(timeout) + Clock::now(), cb);
}

[[maybe_unused]] void HeapTimer::clear() {
    while (!heap_.empty()) heap_.pop();
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
