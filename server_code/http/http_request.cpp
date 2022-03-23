# include "http_request.h"
# include <iostream>

using namespace std;

const unordered_set<string> HttpRequest::kDefaultHtml {
            "/index", "/register", "/login",
             "/welcome", "/video", "/tags", 
             "/author", "/cv", "/contact"
             };

const unordered_set<string> HttpRequest::kArticleTag {
             "/algorithm", "/linux", "/cpp", 
             "/web", "/deeplearning" 
             };

const unordered_map<string, int> HttpRequest::kHtmlTag {
            {"/register.html", 0}, {"/login.html", 1},  {"/contact.html", 2}
            };

const string HttpRequest::kMessageFolder = "/message/";

void HttpRequest::Init() {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

HttpRequest::HttpRequest() {
    Init();
}

bool HttpRequest::is_keep_alive() const {
    if(header_.count("Connection")) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool HttpRequest::Parse(Buffer& buffer) {
    const char CRLF[] = "\r\n";
    if(buffer.ReadableBytes() <= 0) return false;

    while(buffer.ReadableBytes() && state_ != FINISH) {
        const char* line_end = search(buffer.Peek(), buffer.BeginWriteConst(), CRLF, CRLF + 2);
        string line(buffer.Peek(), line_end);

        switch (state_)
        {
        case REQUEST_LINE:
            if(!ParseRequestLine(line)) {
                return false;
            }
            ParsePath();
            break;
        case HEADERS:
            ParseHeader(line);
            if(buffer.ReadableBytes() <= 2) {
                state_ = FINISH;
            }
            break;
        case BODY:
            ParseBody(line);
            break;
        default:
            break;
        }
        if(line_end == buffer.BeginWrite()) break;
        buffer.RetrieveUntil(line_end + 2);
    }

    return true;
}

bool HttpRequest::ParseRequestLine(const string& line) {
    regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    smatch sub_match;
    if(regex_match(line, sub_match, patten)) {   
        method_ = sub_match[1];
        path_ = sub_match[2];
        version_ = sub_match[3];
        state_ = HEADERS;
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}

void HttpRequest::ParsePath() {
    if(path_ == "/") {
        path_ = "/index.html";
    }
    else if(path_ == "/cv") {
        path_ += ".pdf";
    }
    else if(path_.size() > 8 && path_.substr(0, 7) == "/blogs/") {
        path_ = path_ + ".html";
    }
    else {
        for(auto &item: kDefaultHtml) {
            if(item == path_) {
                path_ += ".html";
                return;
            }
        }
        for(auto &item: kArticleTag) {
            if(item == path_) {
                path_ = "/tags" + path_ + ".html";
                return;
            }
        }
    }
}

void HttpRequest::ParseHeader(const string& line) {
    regex patten("^([^:]*): ?(.*)$");
    smatch sub_match;
    if(regex_match(line, sub_match, patten)) {
        header_[sub_match[1]] = sub_match[2];
    }
    else {
        state_ = BODY;
    }
}

void HttpRequest::ParseBody(const string& line) {
    body_ = line;
    ParsePost();
    state_ = FINISH;
    LOG_INFO("Body:%s, len:%d", line.c_str(), line.size());
}

void HttpRequest::ParsePost() {
    if(method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        ParsePostContent();
        if(kHtmlTag.count(path_)) {
            int tag = kHtmlTag.find(path_)->second;
            if(tag == 0 || tag == 1) {
                bool is_login = false;
                if(tag == 1) {
                    is_login = true;
                    post_["admincode"] = "adminmyblog";
                }
                if(UserVerify(post_["username"], post_["password"], post_["admincode"], is_login)) {
                    path_ = "/welcome.html";
                }
                else {
                    path_ = "/error.html";
                }
            }
            else if(tag == 2) {
                SaveMessage(post_["name"], post_["email"], post_["message_content"]);
                path_ = "/index.html";
            }
        }
        else if(path_.size() > 7 && path_.substr(0, 7) == "/blogs/") {
            int begin_pos = path_.find_last_of("/");
            int end_pos = path_.find_last_of(".");
            string blog_name = path_.substr(begin_pos + 1, end_pos - begin_pos - 1);
            SaveComment(blog_name, post_["name"], post_["email"], post_["comment_content"]);
        }
    }
}

void HttpRequest::ParsePostContent() {
    int n = body_.size();
    if(n == 0) return;

    string key, value;

    int i = 0, j = 0;
    for(; i < n; i++) {
        char ch = body_[i];
        switch (ch)
        {
        case '=':
            key = body_.substr(j, i - j);
            j = i + 1;
            break;
        case '&':
            value = body_.substr(j, i - j);
            j = i + 1;
            post_[key] = value;
            break;
        case '+':
            body_[i] = ' ';
            break;
        default:
            break;
        }
    }
    assert(j <= i);
    if(post_.count(key) == 0 && j < i) {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}

bool HttpRequest::UserVerify(const string& client, const string& pwd, const string& code, bool is_login) {
    if(client == "" || pwd == "" || code != "adminmyblog") return false;
    LOG_INFO("Verify username:%s pwd:%s", client.c_str(), pwd.c_str());
    MYSQL* sql;
    ConnectPoolRAII(&sql, ConnectPool::GetInstance()); // get connection
    assert(sql);

    bool flag = false;
    char order[256] = {0};
    // MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;

    if(!is_login) flag = true;

    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", client.c_str());

    if(mysql_query(sql, order)) {
        mysql_free_result(res);
        return false;
    }
    res = mysql_store_result(sql);

    while(MYSQL_ROW row = mysql_fetch_row(res)) {
        string password(row[1]);

        if(is_login) {
            if(pwd == password) flag = true;
            else flag = false; // pwd error
        }
        else flag = false; // client used
    }

    mysql_free_result(res);

    if(!is_login && flag) {
        bzero(order, 256);
        snprintf(order, 256,"INSERT INTO user(username, password) VALUES('%s','%s')", client.c_str(), pwd.c_str());
        if(mysql_query(sql, order)) flag = false;
        else flag = true;
    }
    ConnectPool::GetInstance()->ReleaseConn(sql);;

    return flag;
}

void HttpRequest::SaveMessage(const string& name, const string& email, const string& message_content) {
    Message::GetInstance()->WriteMessage(name, email, message_content);
}

void HttpRequest::SaveComment(const string& blog_name, const string& name, const string& email, const string& comment_content) {
    Comment::GetInstance()->WriteComment(blog_name, name, email, comment_content);
}

string HttpRequest::GetPath() const {
    return path_;
}

string& HttpRequest::GetPath()  {
    return path_;
}

string HttpRequest::GetMethod() const {
    return method_;
}

string HttpRequest::GetVersion() const {
    return version_;
}

string HttpRequest::GetPost(const string& key) const {
    assert(key != "");
    if(post_.count(key)) {
        return post_.find(key)->second;
    }
    return "";
}

string HttpRequest::GetPost(const char* key) const {
    assert(key);
    if(post_.count(key)) {
        return post_.find(key)->second;
    }
    return "";
}