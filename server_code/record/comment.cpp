# include "comment.h"

using namespace std;

Comment::Comment() {
    fp_ = nullptr;
}

Comment::~Comment() {
    if(fp_ != nullptr) {
        lock_guard<mutex> locker(mtx_);
        fclose(fp_);
    }
}

void Comment::Init(const char* path, const char* suffix) {
    path_ = path;
    suffix_ = suffix;
}

Comment* Comment::GetInstance() {
    static Comment instance;
    return &instance;
}

void Comment::WriteComment(const string& blog_name, const string& name, const string& email, const string& comment_content) {
    time_t timer = time(nullptr);
    struct tm* system_time = localtime(&timer);
    struct tm t = *system_time;

    lock_guard<mutex> locker(mtx_);
    {
        snprintf(filename_, 30 ,"%s%s%s", path_, blog_name.c_str(), suffix_);
        fp_ = fopen(filename_, "a");
        if(!fp_) {
            mkdir(path_, 0777);
            fp_ = fopen(filename_, "a");
        }
    }

    assert(fp_);
    {
        char date[20];
        snprintf(date, 20, "[%04d,%02d,%02d]", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
        fputs(date, fp_);

        char content[300];
        snprintf(content, 300, "Name:%s&Email:%s&Comment:%s\r\n", name.c_str(), email.c_str(), comment_content.c_str());
        fputs(content, fp_);
    }
    fclose(fp_);
}