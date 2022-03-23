# ifndef TIMER_H
# define TIMER_H

# include <chrono>
# include <memory>
# include <functional>
# include <cstdint>

using namespace std;

typedef function<void()> TimerTask;
typedef chrono::milliseconds MS;

class Timer {
public:
    Timer(int fd, int execute_time_ms, int interval_ms, const TimerTask& task):
        fd_(fd),
        execute_time_ms_(execute_time_ms),
        interval_ms_(interval_ms),
        task_(task),
        repeated_(false)
        {}

    void Execute() {
        if(task_) task_();
    }

    int GetExecuteTime() const {
        return execute_time_ms_;
    }

    int GetFd() const {
        return fd_;
    }

    void SetRepeated() {
        repeated_ = true;
    }

    void CancelRepeated() {
        repeated_ = false;
    }

    int GetRepeated() const{
        return repeated_;
    }

    void Update() {
        execute_time_ms_ = GetTimeNow() + interval_ms_;
        CancelRepeated();
    }

    static inline int GetTimeNow() {
        using namespace chrono;
        auto now = system_clock::now().time_since_epoch();
        return duration_cast<MS>(now).count();
    }

private:
    int fd_;
    int execute_time_ms_;
    int interval_ms_;
    TimerTask task_;
    bool repeated_;
};

# endif