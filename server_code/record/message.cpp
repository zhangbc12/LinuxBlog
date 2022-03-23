# include "message.h"

using namespace std;

Message::Message() {
    today_ = 0;
    fp_ = nullptr;
}

Message::~Message() {
    if(fp_ != nullptr) {
        lock_guard<mutex> locker(mtx_);
        fclose(fp_);
    }
}

void Message::Init(const char* path, const char* suffix) {
    path_ = path;
    suffix_ = suffix;
    time_t timer = time(nullptr);
    struct tm* system_time = localtime(&timer);
    struct tm t = *system_time;
    snprintf(filename_, 30 ,"%s%04d_%02d_%02d%s", path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
    today_ = t.tm_mday;

    {
        lock_guard<mutex> locker(mtx_);
        if(fp_) fclose(fp_);

        fp_ = fopen(filename_, "a");
        if(!fp_) {
            mkdir(path_, 0777);
            fp_ = fopen(filename_, "a");
        }
        assert(fp_);
        fclose(fp_);
    }
}

Message* Message::GetInstance() {
    static Message instance;
    return &instance;
}

void Message::WriteMessage(const string& name, const string& email, const string& message_content) {
    time_t timer = time(nullptr);
    struct tm* system_time = localtime(&timer);
    struct tm t = *system_time;

    unique_lock<mutex> locker(mtx_);

    if(today_ != t.tm_mday) {
        snprintf(filename_, 30, "%s%04d_%02d_%02d%s", path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
        today_ = t.tm_mday;
    }  

    fp_ = fopen(filename_, "a");
    assert(fp_);

    {
        char timeCur[15];
        snprintf(timeCur, 15, "[%02d:%02d:%02d] ", t.tm_hour, t.tm_min, t.tm_sec);
        fputs(timeCur, fp_);

        char title[100];
        snprintf(title, 100, "Name:%s&Email:%s\r\n", name.c_str(), email.c_str());
        fputs(title, fp_);

        string content = message_content + "\r\n";
        fputs(content.c_str(), fp_);
    }
    fclose(fp_);
}