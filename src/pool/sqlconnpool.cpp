//
// Created by leon on 5/2/23.
//

#include "pool/sqlconnpool.h"

SqlConnPool *SqlConnPool::Instance() {
    static SqlConnPool connPool;
    return &connPool;
}

void SqlConnPool::init(const char *host, unsigned int port,
                       const char *user, const char *pwd, const char *dbName,
                       unsigned int connSize = 10) {
    for (int i = 0; i < connSize; i++) {
        MYSQL *sql = nullptr;
        sql = mysql_init(sql);
        if (!sql) {
            LOG_ERROR("MySql init error!")
            assert(sql);
        }
        sql = mysql_real_connect(sql, host,
                                 user, pwd,
                                 dbName, port, nullptr, 0);
        if (!sql) {
            LOG_ERROR("MySql Connect error!")
        }
        connQueue_.push(sql);
    }
    MAX_CONN_ = connSize;
    sem_init(&semID_, 0, MAX_CONN_);
}

MYSQL *SqlConnPool::getConn() {
    MYSQL *sql;
    if (connQueue_.empty()) {
        LOG_WARN("SqlConnPool busy!")
        return nullptr;
    }
    sem_wait(&semID_);
    {
        std::unique_lock<std::mutex> lock(mux_);
        sql = connQueue_.front();
        connQueue_.pop();
    }
    return sql;
}

void SqlConnPool::freeConn(MYSQL *sql) {
    assert(sql);
    std::unique_lock<std::mutex> lock(mux_);
    connQueue_.push(sql);
    sem_post(&semID_);
}

void SqlConnPool::closePool() {
    std::unique_lock<std::mutex> lock(mux_);
    while (!connQueue_.empty()) {
        auto item = connQueue_.front();
        connQueue_.pop();
        mysql_close(item);
    }
    mysql_library_end();
}

unsigned int SqlConnPool::getFreeConnCount() {
    std::unique_lock<std::mutex> lock(mux_);
    return connQueue_.size();
}

SqlConnPool::~SqlConnPool() {
    closePool();
}
