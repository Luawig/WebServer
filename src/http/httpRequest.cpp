//
// Created by leon on 4/18/23.
//

#include "http/httpRequest.h"

const std::unordered_set<string> HttpRequest::DEFAULT_HTML{
        "/index", "/register", "/login",
        "/welcome", "/video", "/picture",};

const std::unordered_map<string, int> HttpRequest::DEFAULT_HTML_TAG{
        {"/register.html", 0},
        {"/login.html",    1},};

bool HttpRequest::isKeepAlive() const {
    if (version_ != "1.1") {
        return false;
    }
    auto it = header_.find("Connection");
    return (it != header_.end() && it->second == "keep-alive");
}

bool HttpRequest::parse(Buffer &buffer) {
    if (buffer.readableBytes() <= 0) {
        return false;
    }
    const char CRLF[] = "\r\n";
    while (buffer.readableBytes() && state_ != FINISH) {
        const char *lineEnd = std::search(buffer.peek(), buffer.peek() + buffer.readableBytes(), CRLF, CRLF + 2);
        std::string line(buffer.peek(), lineEnd);
        switch (state_) {
            case REQUEST_LINE:
                if (!parseRequestLine_(line)) {
                    return false;
                }
                parsePath_();
                break;
            case HEADERS:
                parseHeader_(line);
                if (buffer.readableBytes() <= 2) {
                    state_ = FINISH;
                }
                break;
            case BODY:
                parseBody_(line);
                break;
            default:
                break;
        }
        if (lineEnd == buffer.peek() + buffer.readableBytes()) {
            break;
        }
        buffer.retrieveUntil(lineEnd + 2);
    }
    return true;
}

void HttpRequest::parsePath_() {
    if (path_ == "/") {
        path_ = "/index.html";
    } else {
        for (auto &item: DEFAULT_HTML) {
            if (item == path_) {
                path_ += ".html";
                break;
            }
        }
    }
}

bool HttpRequest::parseRequestLine_(const std::string &line) {
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if (regex_match(line, subMatch, patten)) {
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        state_ = HEADERS;
        LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str())
        return true;
    }
    LOG_ERROR("RequestLine Error")
    return false;
}

void HttpRequest::parseHeader_(const std::string &line) {
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if (regex_match(line, subMatch, patten)) {
        header_[subMatch[1]] = subMatch[2];
    } else {
        state_ = BODY;
    }
}

void HttpRequest::parseBody_(const std::string &line) {
    body_ = line;
    parsePost_();
    state_ = FINISH;
    LOG_DEBUG("Body: %s, len: %d", line.c_str(), line.size())
}

int HttpRequest::convertHex_(char ch) {
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    return ch;
}

void HttpRequest::parsePost_() {
    if (method_ != "POST" || header_["Content-Type"] != "application/x-www-form-urlencoded") {
        return;
    }
    parseFromUrlencoded_();
    auto it = DEFAULT_HTML_TAG.find(path_);
    if (it != DEFAULT_HTML_TAG.end()) {
        int tag = it->second;
        LOG_DEBUG("Tag: %d (%s)", tag, tag ? "Login" : "Register")
        if (tag == 0 || tag == 1) {
            bool isLogin = (tag == 1);
            if (userVerify_(post_["username"], post_["password"], isLogin)) {
                path_ = "/welcome.html";
            } else {
                path_ = "/error.html";
            }
        }
    }
}

void HttpRequest::parseFromUrlencoded_() {
    if (body_.empty()) {
        return;
    }

    string key, value;
    int num;
    auto n = body_.size();
    int i = 0, j = 0;

    for (; i < n; i++) {
        switch (body_[i]) {
            case '=':
                key = body_.substr(j, i - j);
                j = i + 1;
                break;
            case '+':
                body_[i] = ' ';
                break;
            case '%':
                num = convertHex_(body_[i + 1]) * 16 + convertHex_(body_[i + 2]);
                body_[i + 2] = static_cast<char>(num % 10 + '0');
                body_[i + 1] = static_cast<char>(num / 10 + '0');
                i += 2;
                break;
            case '&':
                value = body_.substr(j, i - j);
                j = i + 1;
                post_[key] = value;
                LOG_DEBUG("%s = %s", key.c_str(), value.c_str())
                break;
            default:
                break;
        }
    }
    assert(j <= i);
    if (post_.count(key) == 0 && j < i) {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}

// Todo: use PreparedStatement
bool HttpRequest::userVerify_(const std::string &name, const std::string &pwd, bool isLogin) {
    if (name.empty() || pwd.empty()) {
        return false;
    }
    LOG_INFO("Verify name: %s pwd: %s", name.c_str(), pwd.c_str())
    MYSQL *sql = SqlConnPool::Instance()->getConn();
    assert(sql);

    bool flag = false;
    char order[256] = {0};
    MYSQL_RES *res = nullptr;

    if (!isLogin) {
        flag = true;
    }
    /* 查询用户及密码 */
    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order)

    if (mysql_query(sql, order)) {
        return false;
    }
    res = mysql_store_result(sql);

    while (MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1])
        string password(row[1]);
        if (isLogin) {
            if (pwd == password) {
                flag = true;
            } else {
                flag = false;
                LOG_DEBUG("pwd error!")
            }
        } else {
            flag = false;
            LOG_DEBUG("user used!")
        }
    }
    mysql_free_result(res);

    if (!isLogin && flag) {
        LOG_DEBUG("register!")
        bzero(order, 256);
        snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG("%s", order)
        if (mysql_query(sql, order)) {
            LOG_DEBUG("Insert error!")
            flag = false;
        } else {
            flag = true;
        }
    }
    SqlConnPool::Instance()->freeConn(sql);
    LOG_DEBUG("userVerify_ success!!")
    return flag;
}
