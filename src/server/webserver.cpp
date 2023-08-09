//
// Created by leon on 5/1/23.
//

#include "server/webserver.h"

WebServer::WebServer() : timer_(new HeapTimer()), threadpool_(new ThreadPool(threadPoolNum_)), epoller_(new Epoller()) {
    char *tmp = getcwd(nullptr, 256);
    srcDir_ = tmp;
    srcDir_ += "/../res";
    free(tmp);
    HttpConnection::userCount = 0;
    HttpConnection::srcDir = srcDir_.c_str();
    SqlConnPool::Instance()->init("localhost", sqlPort_, sqlUser_.c_str(), sqlPasswd_.c_str(), sqlDatabase_.c_str(),
                                  sqlPoolNum_);
    initEventMode_();

    if (!initSocket_()) { isClose_ = true; }

    if (!closeLog_) {
        Log::Instance()->init(logLevel_, "../log", ".log", 1024);
        if (isClose_) { LOG_ERROR("========== Server init error!==========") }
        else {
            LOG_INFO("========== Server init ==========")
            LOG_INFO("Port:%d, OpenLinger: %s", port_, optLinger_ ? "true" : "false")
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                     (listenEvent_ & EPOLLET ? "ET" : "LT"),
                     (connEvent_ & EPOLLET ? "ET" : "LT"))
            LOG_INFO("LogSys level: %d", logLevel_)
            LOG_INFO("srcDir: %s", HttpConnection::srcDir)
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", sqlPoolNum_, threadPoolNum_)
        }
    }
}

WebServer::~WebServer() {
    close(listenfd_);
    isClose_ = true;
    SqlConnPool::Instance()->closePool();
}

void WebServer::parse_args(int argc, char *const *argv) {
    int opt;
    const char *str = "p:l:m:o:s:t:c:a:";
    while ((opt = getopt(argc, argv, str)) != -1) {
        switch (opt) {
            case 'p':
                port_ = std::stoi(optarg);
                break;
            case 'm':
                listenfdMode_ = std::stoi(optarg) & 2;
                connfdMode_ = std::stoi(optarg) & 1;
                break;
            case 'o':
                optLinger_ = std::stoi(optarg);
                break;
            case 's':
                sqlPoolNum_ = std::stoi(optarg);
                break;
            case 't':
                threadPoolNum_ = std::stoi(optarg);
                break;
            case 'c':
                closeLog_ = std::stoi(optarg);
                break;
            default:
                throw std::invalid_argument("unknown args");
        }
    }
}

std::string WebServer::help(const std::string &name) {
    std::stringstream ss;
    ss << "usage: " << name
       << " [-p port] [-l LOGWrite] [-m TRIGMode] [-o OPT_LINGER] [-s sql_num] [-t thread_num] [-c close_log] [-a actor_model]\n";
    return ss.str();
}

void WebServer::run() {
    if (!isClose_)
        LOG_INFO("========== Server start ==========")
    int timeMS;
    while (!isClose_) {
        timeMS = timer_->getNextTick();

        int eventCnt = epoller_->Wait(timeMS);
        for (int i = 0; i < eventCnt; i++) {
            int fd = epoller_->GetEventFd(i);
            uint32_t events = epoller_->GetEvents(i);
            if (fd == listenfd_) {
                dealListen_();
            } else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users_.count(fd) > 0);
                closeConn_(users_[fd]);

            } else if (events & EPOLLIN) {
                assert(users_.count(fd) > 0);
                dealRead_(users_[fd]);
            } else if (events & EPOLLOUT) {
                assert(users_.count(fd) > 0);
                dealWrite_(users_[fd]);
            } else {
                LOG_ERROR("Unexpected event")
            }
        }

    }
}

void WebServer::initEventMode_() {
    listenEvent_ = EPOLLRDHUP | (EPOLLET & listenfdMode_);
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP | (EPOLLET & connfdMode_);
    HttpConnection::isET = connfdMode_;
}

bool WebServer::initSocket_() {
    if (port_ > 65535 || port_ < 1024) {
        LOG_ERROR("Port:%d error!", port_)
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);

    linger optLinger{};
    if (optLinger_) {
        /* 优雅关闭: 直到所剩数据发送完毕或超时 */
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }

    listenfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd_ < 0) {
        LOG_ERROR("Create socket error!", port_)
        return false;
    }

    int ret = setsockopt(listenfd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if (ret < 0) {
        close(listenfd_);
        LOG_ERROR("clear linger error!", port_)
        return false;
    }

    int optval = 1;
    ret = setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval, sizeof(int));
    if (ret == -1) {
        LOG_ERROR("set socket setsockopt error !")
        close(listenfd_);
        return false;
    }

    ret = bind(listenfd_, (struct sockaddr *) &addr, sizeof(addr));
    if (ret < 0) {
        LOG_ERROR("Bind Port:%d error!", port_)
        close(listenfd_);
        return false;
    }

    ret = listen(listenfd_, 6);
    if (ret < 0) {
        LOG_ERROR("Listen port:%d error!", port_)
        close(listenfd_);
        return false;
    }
    ret = epoller_->AddFd(listenfd_, listenEvent_ | EPOLLIN);
    if (ret == 0) {
        LOG_ERROR("Add listen error!")
        close(listenfd_);
        return false;
    }
    setFdNonblock_(listenfd_);
    LOG_INFO("Server port:%d", port_)
    return true;
}

int WebServer::setFdNonblock_(int fd) {
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}

void WebServer::dealListen_() {
    sockaddr_in addr{};
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listenfd_, (struct sockaddr *) &addr, &len);
        if (fd <= 0) { return; }
        else if (HttpConnection::userCount >= MAXFD_) {
            sendError_(fd, "Server busy!");
            LOG_WARN("Clients is full!")
            return;
        }
        addClient_(fd, addr);
    } while (listenEvent_ & EPOLLET);
}

void WebServer::sendError_(int fd, const char *info) {
    auto ret = send(fd, info, strlen(info), 0);
    if (ret < 0) {
        LOG_WARN("send error to client[%d] error!", fd)
    }
    close(fd);
}

void WebServer::addClient_(int fd, sockaddr_in addr) {
    users_[fd] = new HttpConnection(fd, addr);
    timer_->add(fd, timeoutMS_, [this, user = users_[fd]] { closeConn_(user); });

    epoller_->AddFd(fd, EPOLLIN | connEvent_);
    setFdNonblock_(fd);
    LOG_INFO("Client[%d] in!", users_[fd]->getFd())
}

void WebServer::closeConn_(HttpConnection *client) {
    LOG_INFO("Client[%d] quit!", client->getFd())
    epoller_->DelFd(client->getFd());
    auto it = std::find_if(users_.begin(), users_.end(), [client](const auto &pair) {
        return pair.second == client;
    });
    if (it != users_.end()) {
        it->second = nullptr;
        delete client;
    }
}

void WebServer::dealRead_(HttpConnection *client) {
    extentTime_(client);
    threadpool_->addTask([this, client] { onRead_(client); });
}

void WebServer::dealWrite_(HttpConnection *client) {
    extentTime_(client);
    threadpool_->addTask([this, client] { onWrite_(client); });
}

void WebServer::extentTime_(HttpConnection *client) {
    timer_->adjust(client->getFd(), timeoutMS_);
}

void WebServer::onRead_(HttpConnection *client) {
    int readErrno = 0;
    auto ret = client->read(&readErrno);
    if (ret <= 0 && readErrno != EAGAIN) {
        closeConn_(client);
        return;
    }
    onProcess(client);
}

void WebServer::onProcess(HttpConnection *client) {
    if (client->process()) {
        epoller_->ModFd(client->getFd(), connEvent_ | EPOLLOUT);
    } else {
        epoller_->ModFd(client->getFd(), connEvent_ | EPOLLIN);
    }
}

void WebServer::onWrite_(HttpConnection *client) {
    assert(client);
    int writeErrno = 0;
    auto ret = client->write(&writeErrno);
    if (client->writeBytes() == 0) {
        /* 传输完成 */
        if (client->isKeepAlive()) {
            onProcess(client);
            return;
        }
    } else if (ret < 0) {
        if (writeErrno == EAGAIN) {
            /* 继续传输 */
            epoller_->ModFd(client->getFd(), connEvent_ | EPOLLOUT);
            return;
        }
    }
    closeConn_(client);
}