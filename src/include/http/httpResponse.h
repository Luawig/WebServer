//
// Created by leon on 4/18/23.
//

#ifndef WEBSERVER_HTTPRESPONSE_H
#define WEBSERVER_HTTPRESPONSE_H

#include <unordered_map>
#include <fcntl.h>
#include <sys/mman.h>
#include <string>

#include "log/log.h"

class HttpResponse {
public:
    HttpResponse() = default;

    ~HttpResponse() { unmapFile(); }

    void init(const std::string &srcDir, std::string &path, bool isKeepAlive = false, int code = -1);

    void makeResponse(Buffer &buffer);

    void unmapFile();

    char *file() { return mmFile_; }

    [[nodiscard]] size_t fileLen() const {
        return mmFileStat_.st_size;
    }

    void errorContent(Buffer &buffer, const std::string &message) const;

    [[nodiscard]] int Code() const { return code_; }

private:
    void addStateLine_(Buffer &buffer);

    void addHeader_(Buffer &buffer);

    void addContent_(Buffer &buffer);

    void errorHtml_();

    std::string getFileType_();

    int code_{-1};
    bool isKeepAlive_{false};

    std::string path_{};
    std::string srcDir_{};

    char *mmFile_{nullptr};
    struct stat mmFileStat_{0};

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;
};


#endif //WEBSERVER_HTTPRESPONSE_H