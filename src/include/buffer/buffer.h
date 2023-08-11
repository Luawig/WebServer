//
// Created by leon on 4/18/23.
//

#ifndef WEBSERVER_BUFFER_H
#define WEBSERVER_BUFFER_H

#include <string>
#include <vector>
#include <unistd.h>
#include <sys/uio.h>

using std::size_t, std::string;

const size_t BUFFER_SIZE = 65535;

class Buffer {
  public:
    explicit Buffer(unsigned int capacity = 1024);

    ~Buffer() = default;

    size_t append(const string &data);

    size_t append(const char *data, size_t len);

    [[maybe_unused]] size_t append(const Buffer &buffer);

    [[nodiscard]] const char *peek() const {
        return &(*buffer_.begin());
    }

    void retrieve(size_t len);

    void retrieveUntil(const char *end);

    void retrieveAll();

    std::string retrieveAllToStr();

    ssize_t readFd(int fd, int *saveErrno);

    ssize_t writeFd(int fd, int *saveErrno);

    [[nodiscard]] size_t writableBytes() const {
        return buffer_.capacity() - buffer_.size();
    }

    [[nodiscard]] size_t readableBytes() const {
        return buffer_.size();
    }

  private:
    std::vector<char> buffer_;
};

#endif //WEBSERVER_BUFFER_H
