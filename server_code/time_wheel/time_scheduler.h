# ifndef TIME_SCHEDULER_H
# define TIME_SCHEDULER_H

# include <vector>
# include <thread>
# include <mutex>
# include <unordered_set>
# include <unordered_map>
# include <assert.h>

# include "time_wheel.h"

using namespace std;

typedef shared_ptr<TimeWheel> TimeWheelPtr;

class TimeScheduler {
public:
    explicit TimeScheduler(int step_ms = 50, int level = 4);

    void CreateTimerTask(int fd, int time_ms, const TimerTask& task);

    bool Start();

    void Stop();

    TimerPtr GetTimer(int fd);

    void AddTimeWheel(const string& name, int scales, int scale_unit_ms);

private:
    void Run();

    TimeWheelPtr GetFirstTimeWheel();

    TimeWheelPtr GetLastTimeWheel();

    uint32_t step_ms_;

    mutex mtx_;

    thread thread_;

    bool is_stop_;

    vector<TimeWheelPtr> time_wheels_;
    
    unordered_map<int, TimerPtr> ref_;
};

# endif