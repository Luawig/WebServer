//
// Created by leon on 4/18/23.
//

#ifndef WEBSERVER_HTTPREQUEST_H
#define WEBSERVER_HTTPREQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <mysql/mysql.h>

#include "buffer/buffer.h"
#include "log/log.h"
#include "pool/sqlconnpool.h"

class HttpRequest {
  public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };

    enum HTTP_CODE {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };

    HttpRequest() = default;

    ~HttpRequest() = default;

    void clear() {
        method_ = path_ = body_ = "";
        version_ = "1.1";
        state_ = REQUEST_LINE;
        header_.clear();
        post_.clear();
    }

    bool parse(Buffer &buffer);

    std::string &path() {
        return path_;
    }

    std::string method() const {
        return method_;
    }

    std::string &version() {
        return version_;
    }

    std::string getPost(const std::string &key) const {
        if (post_.count(key) == 1) {
            return post_.find(key)->second;
        }
        return "";
    }

    std::string getPost(const char *key) const {
        if (post_.count(key) == 1) {
            return post_.find(key)->second;
        }
        return "";
    }

    bool isKeepAlive() const;

  private:
    bool parseRequestLine_(const std::string &line);

    void parseHeader_(const std::string &line);

    void parseBody_(const std::string &line);

    void parsePath_();

    void parsePost_();

    void parseFromUrlencoded_();

    PARSE_STATE state_{REQUEST_LINE};
    std::string method_{}, path_{}, version_{"1.1"}, body_{};
    std::unordered_map<std::string, std::string> header_{};
    std::unordered_map<std::string, std::string> post_{};

    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;

    static bool userVerify_(const std::string &name, const std::string &pwd, bool isLogin);

    static int convertHex_(char ch);
};


#endif //WEBSERVER_HTTPREQUEST_H