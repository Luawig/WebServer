//
// Created by leon on 4/18/23.
//

#include "http/httpConnection.h"

const char *HttpConnection::srcDir;
std::atomic<int> HttpConnection::userCount;
bool HttpConnection::isET;

HttpConnection::HttpConnection(int fd, const sockaddr_in &addr) : addr_(addr), fd_(fd), isClose_(false) {
    ++userCount;
    writeBuffer_.retrieveAll();
    readBuffer_.retrieveAll();
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, getIp(), getPort(), (int) userCount)
}

HttpConnection::~HttpConnection() {
    response_.unmapFile();
    if (isClose_) {
        return;
    }
    isClose_ = true;
    --userCount;
    close(fd_);
    LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, getIp(), getPort(), (int) userCount)
}

ssize_t HttpConnection::read(int *saveErrno) {
    ssize_t len;
    do {
        len = readBuffer_.readFd(fd_, saveErrno);
    } while (len && isET);
    return len;
}

ssize_t HttpConnection::write(int *saveErrno) {
    ssize_t len_sum = 0;
    do {
        auto len = writev(fd_, iov_, iovCnt_);
        if (len < 0) {
            *saveErrno = errno;
            break;
        }
        if (len > iov_[0].iov_len) {
            iov_[1].iov_base = (uint8_t *) iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);

            if (iov_[0].iov_len) {
                writeBuffer_.retrieveAll();
                iov_[0].iov_len = 0;
            }
        } else {
            iov_[0].iov_base = (uint8_t *) iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
            writeBuffer_.retrieve(len);
        }
        len_sum += len;
    } while (isET || writeBytes() > 10240);
    return len_sum;
}

bool HttpConnection::process() {
    request_.clear();
    if (readBuffer_.readableBytes() <= 0) {
        return false;
    } else if (request_.parse(readBuffer_)) {
        LOG_DEBUG("%s", request_.path().c_str())
        response_.init(srcDir, request_.path(), request_.version(), request_.isKeepAlive(), 200);
    } else {
        response_.init(srcDir, request_.path(), request_.version(), false, 400);
    }

    // 生成响应头
    response_.makeResponse(writeBuffer_);

    iov_[0].iov_base = const_cast<char *>(writeBuffer_.peek());
    iov_[0].iov_len = writeBuffer_.readableBytes();
    iovCnt_ = 1;
    if (response_.fileLen() > 0 && response_.file()) {
        iov_[1].iov_base = response_.file();
        iov_[1].iov_len = response_.fileLen();
        iovCnt_ = 2;
    }
    LOG_DEBUG("filesize:%d, %d  to %d", response_.fileLen(), iovCnt_, writeBytes())
    return true;
}
