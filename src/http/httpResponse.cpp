//
// Created by leon on 4/18/23.
//

#include "http/httpResponse.h"

const std::unordered_map<string, string> HttpResponse::SUFFIX_TYPE = {
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

const std::unordered_map<int, string> HttpResponse::CODE_STATUS = {
        {200, "OK"},
        {400, "Bad Request"},
        {403, "Forbidden"},
        {404, "Not Found"},
};

const std::unordered_map<int, string> HttpResponse::CODE_PATH = {
        {400, "/400.html"},
        {403, "/403.html"},
        {404, "/404.html"},
};

void HttpResponse::init(const std::string &srcDir, string &path, std::string &version, bool isKeepAlive, int code) {
    assert(!srcDir.empty());
    if (mmapFile_) {
        unmapFile();
    }
    code_ = code;
    isKeepAlive_ = isKeepAlive;
    path_ = path;
    version_ = version;
    srcDir_ = srcDir;
    mmapFile_ = nullptr;
    fileStat_ = {0};
}

void HttpResponse::makeResponse(Buffer &buffer) {
    if (stat((srcDir_ + path_).c_str(), &fileStat_) < 0 || S_ISDIR(fileStat_.st_mode)) {
        code_ = 404;
    } else if (!(fileStat_.st_mode & S_IROTH)) {
        code_ = 403;
    }

    errorHtml_();
    addStateLine_(buffer);
    addHeader_(buffer);
    addContent_(buffer);
}

void HttpResponse::errorHtml_() {
    auto it = CODE_PATH.find(code_);
    if (it != CODE_PATH.end()) {
        path_ = it->second;
        stat((srcDir_ + path_).c_str(), &fileStat_);
    }
}

void HttpResponse::addStateLine_(Buffer &buffer) {
    string status;
    auto it = CODE_STATUS.find(code_);
    if (it == CODE_STATUS.end()) {
        LOG_WARN("Response code error!")
        code_ = 400;
        status = CODE_STATUS.find(400)->second;
    } else {
        status = it->second;
    }
    buffer.append("HTTP/" + version_ + " " + std::to_string(code_) + " " + status + "\r\n");
}

void HttpResponse::addHeader_(Buffer &buffer) {
    buffer.append("Connection: ");
    if (isKeepAlive_) {
        buffer.append("keep-alive\r\n");
        buffer.append("keep-alive: max=6, timeout=120\r\n");
    } else {
        buffer.append("close\r\n");
    }
    buffer.append("Content-type: " + getFileType_() + "\r\n");
}

void HttpResponse::addContent_(Buffer &buffer) {
    int srcFd = open((srcDir_ + path_).c_str(), O_RDONLY);
    if (srcFd < 0) {
        LOG_WARN("open file error! file: %s", (srcDir_ + path_).c_str())
        errorContent(buffer, "file NotFound!");
        return;
    }

    /* 将文件映射到内存提高文件的访问速度 
        MAP_PRIVATE 建立一个写入时拷贝的私有映射*/
    LOG_DEBUG("filepath: %s", (srcDir_ + path_).c_str())
    auto ret = mmap(nullptr, fileStat_.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if (ret == MAP_FAILED) {
        LOG_ERROR("mmap error! file: %s", (srcDir_ + path_).c_str())
        errorContent(buffer, "file NotFound!");
        return;
    }
    mmapFile_ = (char *) ret;
    close(srcFd);
    buffer.append("Content-length: " + std::to_string(fileStat_.st_size) + "\r\n\r\n");
}

void HttpResponse::unmapFile() {
    if (mmapFile_) {
        munmap(mmapFile_, fileStat_.st_size);
        mmapFile_ = nullptr;
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
    body += std::to_string(code_) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buffer.append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buffer.append(body);
}
