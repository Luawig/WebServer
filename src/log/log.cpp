//
// Created by leon on 5/3/23.
//

#include "log/log.h"

Log::Log() {
    lineCount_ = 0;
    isAsync_ = false;
    writeThread_ = nullptr;
    deque_ = nullptr;
    toDay_ = 0;
    fp_ = nullptr;
}

Log::~Log() {
    if (writeThread_ && writeThread_->joinable()) {
        while (!deque_->empty()) {
            deque_->flush();
        }
        deque_->Close();
        writeThread_->join();
    }
    if (fp_) {
        std::lock_guard<std::mutex> lock(lock_);
        flush();
        fclose(fp_);
    }
}

int Log::GetLevel() {
    std::lock_guard<std::mutex> lock(lock_);
    return level_;
}

void Log::SetLevel(int level) {
    std::lock_guard<std::mutex> lock(lock_);
    level_ = level;
}

void Log::init(int level = 1, const char *path, const char *suffix,
               int maxQueueSize) {
    isOpen_ = true;
    SetLevel(level);
    if (maxQueueSize > 0) {
        isAsync_ = true;
        if (!deque_) {
            deque_ = std::make_unique<BlockDeque<std::string>>();
            writeThread_ = std::make_unique<std::thread>(FlushLogThread);
        }
    } else {
        isAsync_ = false;
    }

    lineCount_ = 0;

    time_t timer = time(nullptr);
    struct tm *sysTime = localtime(&timer);
    struct tm t = *sysTime;
    path_ = path;
    suffix_ = suffix;
    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s",
             path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
    toDay_ = t.tm_mday;

    {
        std::lock_guard<std::mutex> lock(lock_);
        buff_.retrieveAll();
        if (fp_) {
            flush();
            fclose(fp_);
        }

        fp_ = fopen(fileName, "a");
        if (fp_ == nullptr) {
            mkdir(path_, 0777);
            fp_ = fopen(fileName, "a");
        }
        assert(fp_ != nullptr);
    }
}

void Log::write(int level, const char *format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t tSec = now.tv_sec;
    struct tm *sysTime = localtime(&tSec);
    struct tm t = *sysTime;
    va_list vaList;

    std::unique_lock<std::mutex> lock(lock_);

    if (toDay_ != t.tm_mday || (lineCount_ && (lineCount_ % MAX_LINES == 0))) {

        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

        if (toDay_ != t.tm_mday) {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail, suffix_);
            toDay_ = t.tm_mday;
            lineCount_ = 0;
        } else {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, tail, (lineCount_ / MAX_LINES), suffix_);
        }

        flush();
        fclose(fp_);
        fp_ = fopen(newFile, "a");
        assert(fp_ != nullptr);
    }

    lineCount_++;

    char buf[LOG_LEN];
    int n = snprintf(buf, LOG_LEN, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                     t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                     t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
    buff_.append(buf, n);

    AppendLogLevelTitle_(level);

    va_start(vaList, format);

    int m = vsnprintf(buf, buff_.writableBytes(), format, vaList);
    buff_.append(buf, m);
    va_end(vaList);

    buff_.append("\n\0");

    if (isAsync_ && deque_ && !deque_->full()) {
        deque_->push_back(buff_.retrieveAllToStr());
    } else {
        fputs(buff_.peek(), fp_);
    }
    buff_.retrieveAll();
}

void Log::AppendLogLevelTitle_(int level) {
    switch (level) {
        case 0:
            buff_.append("[debug]: ");
            break;
        case 1:
            buff_.append("[info] : ");
            break;
        case 2:
            buff_.append("[warn] : ");
            break;
        case 3:
            buff_.append("[error]: ");
            break;
        default:
            buff_.append("[info] : ");
            break;
    }
}

void Log::flush() {
    if (isAsync_) {
        deque_->flush();
    }
    fflush(fp_);
}

void Log::AsyncWrite_() {
    string str;
    while (deque_->pop(str)) {
        std::lock_guard<std::mutex> lock(lock_);
        fputs(str.c_str(), fp_);
    }
}

Log *Log::Instance() {
    static Log inst;
    return &inst;
}

void Log::FlushLogThread() {
    Log::Instance()->AsyncWrite_();
}