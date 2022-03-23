# ifndef LOG_H
# define LOG_H

# include <mutex>
# include <thread>
# include <stdarg.h>
# include <sys/stat.h>
# include "block_queue.h"
# include "../buffer/buffer.h"

using namespace std;

class Log {
public:
    void Init(int level, const char* path = "./log/", const char* suffix = ".log", int max_queue_size = 1024);

    static Log* GetInstance();

    static void FlushLog();

    void Write(int level, const char *format, ...);

    void FlushQueue();

    int GetLevel();

    bool GetOpen();

private:
    Log();

    ~Log();

    void AppendLevelToTitle(int level);

    void AsyncWrite();

    static const int kLogNameLen = 256;
    static const int kMaxLines = 50000;

    const char* path_;
    const char* suffix_;

    int line_count_;

    int today_;

    bool is_open_;

    Buffer buff_;

    int level_;

    bool async_;

    FILE* fp_;

    unique_ptr<BlockQueue<string>> que_;
    unique_ptr<thread> write_thread_;
    mutex mtx_;
};

# define LOG_BASE(level, format, ...)  \
    do {\
        Log* log = Log::GetInstance();\
        if(log->GetOpen() && log->GetLevel() <= level) {\
            log->Write(level, format, ##__VA_ARGS__);\
            log->FlushQueue();\
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

# endif