//
// Created by leon on 4/18/23.
//
#include "buffer/buffer.h"

Buffer::Buffer(const unsigned int capacity) {
    buffer_.resize(capacity);
}

size_t Buffer::append(const char *data, size_t len) {
    std::copy(data, data + len, std::back_inserter(buffer_));
    return len;
}

size_t Buffer::append(const std::string &data) {
    append(data.c_str(), data.length());
    return data.length();
}

size_t Buffer::append(const Buffer &buffer) {
    buffer_.insert(buffer_.end(), buffer.buffer_.begin(), buffer.buffer_.end());
    return buffer.buffer_.size();
}

void Buffer::retrieve(size_t len) {
    len = std::min(len, readableBytes());
    buffer_.erase(buffer_.begin(), buffer_.begin() + len);
}

void Buffer::retrieveUntil(const char *end) {
    if (peek() > end) return;
    retrieve(end - peek());
}

void Buffer::retrieveAll() {
    buffer_.clear();
}

std::string Buffer::retrieveAllToStr() {
    std::string str(peek(), readableBytes());
    retrieveAll();
    return str;
}

ssize_t Buffer::readFd(int fd, int *saveErrno) {
    char buff[BUFFER_SIZE];

    const ssize_t len = read(fd, buff, BUFFER_SIZE);
    if (len == -1) {
        *saveErrno = errno;
        return -1;
    }
    append(buff, len);

    return len;
}

ssize_t Buffer::writeFd(int fd, int *Errno) {
    ssize_t len = write(fd, peek(), readableBytes());
    if (len < 0) {
        *Errno = errno;
        return len;
    }
    retrieve(len);
    return len;
}

