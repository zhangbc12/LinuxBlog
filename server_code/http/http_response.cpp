# include "http_response.h"
# include <iostream>

using namespace std;

const unordered_map<string, string> HttpResponse::kSuffixType = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const unordered_map<int, string> HttpResponse::kCodeStatus = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

const unordered_map<int, string> HttpResponse::kCodePath = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};

HttpResponse::HttpResponse() {
    code_ = -1;
    path_ = "";
    src_dir_ = "";
    is_keep_alive_ = false;
    mm_file_ = nullptr;
    mm_file_stat_ = {0};
}

HttpResponse::~HttpResponse() {
    UnmapFile();
}

void HttpResponse::UnmapFile() {
    if(mm_file_) {
        munmap(mm_file_, mm_file_stat_.st_size);
        mm_file_ = nullptr;
    }
}


void HttpResponse::Init(const string& src_dir, string& path, bool is_keep_alive, int code)  {
    assert(src_dir != "");
    if(mm_file_) UnmapFile();
    code_ = code;
    is_keep_alive_ = is_keep_alive;
    path_ = path;
    src_dir_ = src_dir;
    mm_file_ = nullptr;
    mm_file_stat_ = {0};
}

void HttpResponse::MakeResponse(Buffer& buffer) {
    if(stat((src_dir_ + path_).data(), &mm_file_stat_) < 0 || S_ISDIR(mm_file_stat_.st_mode)) {
        code_ = 404;
    }
    else if(!(mm_file_stat_.st_mode & S_IROTH)) {
        code_ = 403;
    }
    else if(code_ == -1) {
        code_ = 200;
    }
    ErrorHtml();
    AddStateLine(buffer);
    AddHeader(buffer);
    AddContent(buffer);
}

void HttpResponse::ErrorHtml() {
    if(kCodePath.count(code_) == 1) {
        path_ = kCodePath.find(code_)->second;
        stat((src_dir_ + path_).data(), &mm_file_stat_);
    }
}

void HttpResponse::AddStateLine(Buffer& buffer) {
    string status;
    if(kCodeStatus.count(code_)) {
        status = kCodeStatus.find(code_)->second;
    }
    else {
        code_ = 400;
        status = kCodeStatus.find(400)->second;
    }
    buffer.Append("HTTP/1.1 " + to_string(code_) + " " + status + "\r\n");
}

void HttpResponse::AddHeader(Buffer& buffer) {
    buffer.Append("Connection: ");
    if(is_keep_alive_) {
        buffer.Append("keep-alive\r\n");
        buffer.Append("keep-alive: max=6, timeout=120\r\n");
    }
    else {
        buffer.Append("close\r\n");
    }
    buffer.Append("Content-type: " + GetFileType() + "\r\n");
}

void HttpResponse::AddContent(Buffer& buffer) {
    FILE *fp = fopen((src_dir_ + path_).data(), "r");
    if(!fp) {
        ErrorContent(buffer, "File Not Found!");
        return;
    }
    int src_fd = open((src_dir_ + path_).data(), O_RDONLY);
    if(src_fd < 0) {
        ErrorContent(buffer, "File Not Found!");
        return;
    }

    int *mm_ret = (int*)mmap(0, mm_file_stat_.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
    if(*mm_ret == -1) {
        ErrorContent(buffer, "File Not Found!");
        return;
    }

    mm_file_ = (char*)mm_ret;
    close(src_fd);

    buffer.Append("Content-length: " + to_string(mm_file_stat_.st_size) + "\r\n\r\n");
    fclose(fp);
}

string HttpResponse::GetFileType() {
    string::size_type idx = path_.find_last_of('.');
    if(idx == string::npos) {
        return "text/plain";
    }
    string suffix = path_.substr(idx);
    if(kSuffixType.count(suffix)) {
        return kSuffixType.find(suffix)->second;
    }
    return "text/plain";
}

void HttpResponse::ErrorContent(Buffer& buffer, string message) {
    string body;
    string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    
    if(kCodeStatus.count(code_)) {
        status = kCodeStatus.find(code_)->second;
    }
    else {
        status = "Bad Request";
    }

    body += to_string(code_) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>Server</em></body></html>";

    buffer.Append("Content-length: " + to_string(body.size()) + "\r\n\r\n");
    buffer.Append(body);
}

char* HttpResponse::File() {
    return mm_file_;
}

size_t HttpResponse::Filelen() const {
    return mm_file_stat_.st_size;
}

int HttpResponse::Code() const {
    return code_;
}