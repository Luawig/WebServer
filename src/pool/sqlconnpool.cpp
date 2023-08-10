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
                       unsigned int connNum) {
    for (int i = 0; i < connNum; i++) {
        auto sql = mysql_init(nullptr);
        if (!sql) {
            LOG_ERROR("MySql init error!")
        }
        sql = mysql_real_connect(sql, host,
                                 user, pwd,
                                 dbName, port, nullptr, 0);
        if (!sql) {
            LOG_ERROR("MySql Connect error!")
        }
        connQueue_.push(sql);
    }
}

MYSQL *SqlConnPool::getConn() {
    std::unique_lock<std::mutex> lock(lock_);
    if (connQueue_.empty()) {
        LOG_WARN("SqlConnPool busy!")
        cv_.wait(lock);
    }
    if (connQueue_.empty()) {
        LOG_WARN("Get MySqlConn error!")
        return nullptr;
    }
    MYSQL *sql = connQueue_.front();
    connQueue_.pop();
    return sql;
}

void SqlConnPool::freeConn(MYSQL *sql) {
    assert(sql);
    {
        std::unique_lock<std::mutex> lock(lock_);
        connQueue_.push(sql);
    }
    cv_.notify_one();
}

void SqlConnPool::closePool() {
    std::unique_lock<std::mutex> lock(lock_);
    while (!connQueue_.empty()) {
        auto item = connQueue_.front();
        connQueue_.pop();
        mysql_close(item);
    }
    mysql_library_end();
}

[[maybe_unused]] unsigned int SqlConnPool::getFreeConnNum() {
    std::unique_lock<std::mutex> lock(lock_);
    return connQueue_.size();
}

SqlConnPool::~SqlConnPool() {
    closePool();
}
