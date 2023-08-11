//
// Created by leon on 4/18/23.
//

#include "http/httpResponse.h"

using namespace std;

const unordered_map<string, string> HttpResponse::SUFFIX_TYPE = {
        {".html",  "text/html"},
        {".xml",   "text/xml"},
        {".xhtml", "application/xhtml+xml"},
        {".txt",   "text/plain"},
        {".rtf",   "application/rtf"},
        {".pdf",   "application/pdf"},
        {".word",  "application/nsword"},
        {".png",   "image/png"},
        {".gif",   "image/gif"},
        {".jpg",   "image/jpeg"},
        {".jpeg",  "image/jpeg"},
        {".au",    "audio/basic"},
        {".mpeg",  "video/mpeg"},
        {".mpg",   "video/mpeg"},
        {".avi",   "video/x-msvideo"},
        {".gz",    "application/x-gzip"},
        {".tar",   "application/x-tar"},
        {".css",   "text/css "},
        {".js",    "text/javascript "},
};

const unordered_map<int, string> HttpResponse::CODE_STATUS = {
        {200, "OK"},
        {400, "Bad Request"},
        {403, "Forbidden"},
        {404, "Not Found"},
};

const unordered_map<int, string> HttpResponse::CODE_PATH = {
        {400, "/400.html"},
        {403, "/403.html"},
        {404, "/404.html"},
};

void HttpResponse::init(const std::string &srcDir, string &path, bool isKeepAlive, int code) {
    assert(!srcDir.empty());
    if (mmFile_) {
        unmapFile();
    }
    code_ = code;
    isKeepAlive_ = isKeepAlive;
    path_ = path;
    srcDir_ = srcDir;
    mmFile_ = nullptr;
    mmFileStat_ = {0};
}

void HttpResponse::makeResponse(Buffer &buffer) {
    /* 判断请求的资源文件 */
    if (stat((srcDir_ + path_).c_str(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode)) {
        code_ = 404;
    } else if (!(mmFileStat_.st_mode & S_IROTH)) {
        code_ = 403;
    } else if (code_ == -1) {
        code_ = 200;
    }
    errorHtml_();
    addStateLine_(buffer);
    addHeader_(buffer);
    addContent_(buffer);
}

void HttpResponse::errorHtml_() {
    if (CODE_PATH.count(code_) == 1) {
        path_ = CODE_PATH.find(code_)->second;
        stat((srcDir_ + path_).c_str(), &mmFileStat_);
    }
}

void HttpResponse::addStateLine_(Buffer &buffer) {
    string status;
    if (CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    } else {
        code_ = 400;
        status = CODE_STATUS.find(400)->second;
    }
    buffer.append("HTTP/1.1 " + to_string(code_) + " " + status + "\r\n");
}

void HttpResponse::addHeader_(Buffer &buffer) {
    buffer.append("Connection: ");
    if (isKeepAlive_) {
        buffer.append("keep-alive\r\n");
        buffer.append("keep-alive: max=6, timeout=120\r\n");
    } else {
        buffer.append("Close\r\n");
    }
    buffer.append("Content-type: " + getFileType_() + "\r\n");
}

void HttpResponse::addContent_(Buffer &buffer) {
    int srcFd = open((srcDir_ + path_).c_str(), O_RDONLY);
    if (srcFd < 0) {
        errorContent(buffer, "file NotFound!");
        return;
    }

    /* 将文件映射到内存提高文件的访问速度 
        MAP_PRIVATE 建立一个写入时拷贝的私有映射*/
    LOG_DEBUG("filepath: %s", (srcDir_ + path_).c_str())
    int *mmRet = (int *) mmap(nullptr, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if (*mmRet == -1) {
        errorContent(buffer, "file NotFound!");
        return;
    }
    mmFile_ = (char *) mmRet;
    close(srcFd);
    buffer.append("Content-length: " + to_string(mmFileStat_.st_size) + "\r\n\r\n");
}

void HttpResponse::unmapFile() {
    if (mmFile_) {
        munmap(mmFile_, mmFileStat_.st_size);
        mmFile_ = nullptr;
    }
}

string HttpResponse::getFileType_() {
    /* 判断文件类型 */
    string::size_type idx = path_.find_last_of('.');
    if (idx == string::npos) {
        return "text/plain";
    }
    string suffix = path_.substr(idx);
    if (SUFFIX_TYPE.count(suffix) == 1) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

void HttpResponse::errorContent(Buffer &buffer, const std::string &message) const {
    string body;
    string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if (CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    } else {
        status = "Bad Request";
    }
    body += to_string(code_) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buffer.append("Content-length: " + to_string(body.size()) + "\r\n\r\n");
    buffer.append(body);
}
