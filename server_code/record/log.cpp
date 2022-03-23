# include "log.h"
# include <iostream>

using namespace std;

Log::Log(): 
    line_count_(0),
    today_(0),
    async_(false),
    fp_(nullptr),
    que_(nullptr),
    write_thread_(nullptr)
    {}

Log::~Log() {
    if(write_thread_ != nullptr && write_thread_->joinable()) {
        while(!que_->Empty()) {
            que_->Flush();
        }
        que_->Close();
        write_thread_->join();
    }
    if(fp_ != nullptr) {
        lock_guard<mutex> locker(mtx_);
        FlushQueue();
        fclose(fp_);
    }
}

void Log::Init(int level = 1, const char* path, const char* suffix, int max_queue_size) {
    is_open_ = true;
    level_ = level;
    if(max_queue_size > 0) {
        async_ = true;
        if(!que_) {
            unique_ptr<BlockQueue<string>> temp_que(new BlockQueue<string> (max_queue_size));
            que_ = move(temp_que);

            unique_ptr<thread> temp_thread(new thread(FlushLog));
            write_thread_ = move(temp_thread);
        }
    }
    else {
        async_ = false;
    }

    line_count_ = 0;

    time_t timer = time(nullptr);
    struct tm* system_time = localtime(&timer);
    struct tm t = *system_time;

    path_ = path;
    suffix_ = suffix;
    char filename[kLogNameLen] = {0};
    snprintf(filename, kLogNameLen - 1, "%s/%04d_%02d_%02d%s",
            path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
    today_ = t.tm_mday;

    lock_guard<mutex> locker(mtx_);
    buff_.RetrieveAll();
    if(fp_) {
        FlushQueue();
        fclose(fp_);
    }

    fp_ = fopen(filename, "a");
    if(fp_ == nullptr) {
        mkdir(path_, 0777);
        fp_ = fopen(filename, "a");
    }
    assert(fp_);
}

Log* Log::GetInstance() {
    static Log log;

    return &log;
}

void Log::FlushLog() {
    GetInstance()->AsyncWrite();
}

void Log::Write(int level, const char *format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t second_now = now.tv_sec;
    struct tm *system_time = localtime(&second_now);
    struct tm t = *system_time;
    va_list list;

    /* 日志日期 日志行s数 */
    if (today_ != t.tm_mday || (line_count_ && (line_count_  %  kMaxLines == 0))) {
        unique_lock<mutex> locker(mtx_);
        locker.unlock();
        
        char filename[kLogNameLen] = {0};
        char log_name[36] = {0};
        snprintf(log_name, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

        if (today_ != t.tm_mday)
        {
            snprintf(filename, kLogNameLen - 72, "%s/%s%s", path_, log_name, suffix_);
            today_ = t.tm_mday;
            line_count_ = 0;
        }
        else {
            snprintf(filename, kLogNameLen - 72, "%s/%s-%d%s", path_, log_name, (line_count_  / kMaxLines), suffix_);
        }
        
        locker.lock();
        FlushQueue();
        fclose(fp_);
        fp_ = fopen(filename, "a");
        assert(fp_ != nullptr);
    }
    
    unique_lock<mutex> locker(mtx_);
    line_count_++;
    int n = snprintf(buff_.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
                
    buff_.HasWritten(n);
    AppendLevelToTitle(level);

    va_start(list, format);
    int m = vsnprintf(buff_.BeginWrite(), buff_.WritableBytes(), format, list);
    va_end(list);

    buff_.HasWritten(m);
    buff_.Append("\n\0", 2);

    if(async_ && que_ && !que_->Full()) {
        que_->Push(buff_.RetrieveAllToStr());
    } else {
        fputs(buff_.Peek(), fp_);
    }
    buff_.RetrieveAll();
    
}

void Log::FlushQueue() {
    if(async_) {
        que_->Flush();
    }
    fflush(fp_);
}

int Log::GetLevel() {
    lock_guard<mutex> locker(mtx_);
    return level_;
}

bool Log::GetOpen() {
    lock_guard<mutex> locker(mtx_);
    return is_open_;
}

void Log::AppendLevelToTitle(int level) {
    switch(level) {
    case 0:
        buff_.Append("[debug]: ", 9);
        break;
    case 1:
        buff_.Append("[info] : ", 9);
        break;
    case 2:
        buff_.Append("[warn] : ", 9);
        break;
    case 3:
        buff_.Append("[error]: ", 9);
        break;
    default:
        buff_.Append("[info] : ", 9);
        break;
    }
}

void Log::AsyncWrite() {
    string str = "";
    while(que_->Pop(str)) {
        lock_guard<mutex> locker(mtx_);
        fputs(str.c_str(), fp_);
    }
}  