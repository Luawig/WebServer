//
// Created by leon on 5/2/23.
//

#ifndef WEBSERVER_SQLCONNPOOL_H
#define WEBSERVER_SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <mutex>
#include <queue>
#include <semaphore.h>

#include "log/log.h"

class SqlConnPool {
public:
    static SqlConnPool *Instance();

    MYSQL *getConn();

    void freeConn(MYSQL *conn);

    unsigned int getFreeConnCount();

    void init(const char *host, unsigned int port,
              const char *user, const char *pwd,
              const char *dbName, unsigned int connSize);

    void closePool();

private:
    SqlConnPool() = default;

    ~SqlConnPool();

    unsigned int MAX_CONN_{};

    std::queue<MYSQL *> connQueue_;
    std::mutex mux_;
    sem_t semID_{};
};


#endif //WEBSERVER_SQLCONNPOOL_H
