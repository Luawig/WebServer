//
// Created by leon on 4/18/23.
//


#ifndef WEBSERVER_HTTPCONNECTION_H
#define WEBSERVER_HTTPCONNECTION_H

#include <sys/uio.h>
#include <arpa/inet.h>
#include <atomic>

#include "log/log.h"
#include "buffer/buffer.h"
#include "httpRequest.h"
#include "httpResponse.h"

class HttpConnection {
public:
    HttpConnection(int fd, const sockaddr_in &addr);

    ~HttpConnection();

    ssize_t read(int *saveErrno);

    ssize_t write(int *saveErrno);

    int getFd() const {
        return fd_;
    }

    int getPort() const {
        return addr_.sin_port;
    }

    const char *getIp() const {
        return inet_ntoa(addr_.sin_addr);
    }

    sockaddr_in getAddr() const {
        return addr_;
    }

    bool process();

    unsigned int writeBytes() {
        return iov_[0].iov_len + iov_[1].iov_len;
    }

    bool isKeepAlive() const {
        return request_.isKeepAlive();
    }

    static bool isET;
    static const char *srcDir;
    static std::atomic<int> userCount;

private:
    int fd_{-1};
    struct sockaddr_in addr_{0};

    bool isClose_{true};

    int iovCnt_{};
    struct iovec iov_[2]{};

    Buffer readBuffer_;
    Buffer writeBuffer_;

    HttpRequest request_;
    HttpResponse response_;
};


#endif //WEBSERVER_HTTPCONNECTION_H