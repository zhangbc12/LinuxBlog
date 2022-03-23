# ifndef TIME_WHEEL_H
# define TIME_WHEEL_H

# include <vector>
# include <string>

# include "timer.h"

using namespace std;

typedef shared_ptr<Timer> TimerPtr;

class TimeWheel {
public:
    TimeWheel(const string& name, int scales, int scales_unit_ms);

    int GetIndex() const {
        return time_pos_;
    }

    void SetNextTimeWheel(TimeWheel* next_time_wheel) {
        next_ = next_time_wheel;
    }

    void SetPrevTimeWheel(TimeWheel* prev_time_wheel) {
        prev_ = prev_time_wheel;
    }

    int GetCurrentTime() const;

    void InsertTimer(TimerPtr timer);

    void Tick();

    vector<TimerPtr> MoveCurrentSlot();
private:
    string name_;
    int time_pos_;

    int scales_;
    int step_ms_;

    vector<vector<TimerPtr>> slots_;

    TimeWheel* next_;
    TimeWheel* prev_;
};

# endif