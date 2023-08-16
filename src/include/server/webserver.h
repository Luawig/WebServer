//
// Created by leon on 5/1/23.
//

#ifndef WEBSERVER_WEBSERVER_H
#define WEBSERVER_WEBSERVER_H

#include <csignal>
#include <filesystem>
#include <getopt.h>
#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>

#include "log/log.h"
#include "timer/heaptimer.h"
#include "pool/threadpool.h"
#include "http/httpConnection.h"
#include "server/epoller.h"

class WebServer {
  public:
    explicit WebServer(int argc, char *const *argv);

    ~WebServer();

    // 处理命令行参数
    void parse_args(int argc, char *const *argv);

    // 命令行参数帮助
    static string help(const string &name);

    // 运行
    void run();

  private:

    bool initSocket_();

    void addClient_(int fd, sockaddr_in addr);

    void dealListen_();

    void dealWrite_(HttpConnection *client);

    void dealRead_(HttpConnection *client);

    void extendTime_(HttpConnection *client);

    void closeConn_(HttpConnection *client);

    void onRead_(HttpConnection *client);

    void onWrite_(HttpConnection *client);

    void onProcess(HttpConnection *client);

    static int setFdNonblock_(int fd);

    static void sendError_(int fd, const char *info);

    // 端口号
    unsigned int port_{10086};

    // 关闭 socket 时是否等待
    bool optLinger_{false};

    // 数据库连接池数量
    unsigned int sqlPoolNum_{8};

    // 线程池连接数量
    unsigned int threadPoolNum_{8};

    // 是否关闭日志，0 打开日志，1 关闭日志
    bool closeLog_{false};

    // 日志等级
    int logLevel_{0};

    // mysql 端口
    unsigned int sqlPort_{3306};

    // mysql 用户名
    string sqlUser_{"leon"};

    // mysql 密码
    string sqlPasswd_{"leon"};

    // mysql 数据库
    string sqlDatabase_{"webserver_demo1"};

    // 资源路径
    char *srcDir_;

    // 日志路径
    char *srcLog_;

    // 超时时间
    int timeoutMS_{60000};

    //
    int listenfd_{};

    //
    bool isClose_{false};

    uint32_t listenTrigMode_{EPOLLRDHUP};
    uint32_t connTrigMode_{EPOLLONESHOT | EPOLLRDHUP};

    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConnection *> users_;

    static const unsigned int MAXFD_ = 65535;
};


#endif //WEBSERVER_WEBSERVER_H
