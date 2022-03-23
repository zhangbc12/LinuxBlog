# include "time_scheduler.h"

using namespace std;

TimeScheduler::TimeScheduler(int step_ms, int level) : 
    step_ms_(step_ms), 
    is_stop_(false) {
        assert(level > 0);
        if(level > 3) {
            AddTimeWheel("Hour", 24, 60 * 60 * 1000);
        }
        if(level > 2) {
            AddTimeWheel("Minute", 60, 60 * 1000);
        } 
        if(level > 1) {
            AddTimeWheel("Second", 60, 1000);
        }
        AddTimeWheel("Millisecond", 1000 / step_ms, step_ms);
    }

void TimeScheduler::CreateTimerTask(int fd, int time_ms, const TimerTask& task) {
    if (time_wheels_.empty()) {
        return;
    }

    lock_guard<mutex> locker(mtx_);
    int execute_time_ms = Timer::GetTimeNow() + time_ms;
    TimerPtr timer(new Timer(fd, execute_time_ms, time_ms, task));
    ref_[fd] = timer;
    GetFirstTimeWheel()->InsertTimer(timer);
}

bool TimeScheduler::Start() {
    if(step_ms_ < 50 || time_wheels_.size() == 0) return false;

    thread_ = thread(bind(&TimeScheduler::Run, this));
    return true;
}

void TimeScheduler::Stop() {
    lock_guard<mutex> locker(mtx_);
    is_stop_ = true;
}

TimerPtr TimeScheduler::GetTimer(int fd) {
    return ref_[fd];
}

void TimeScheduler::AddTimeWheel(const string& name, int scales, int scale_unit_ms) {
    TimeWheelPtr time_wheel = make_shared<TimeWheel>(name, scales, scale_unit_ms);
    if(time_wheels_.size() == 0) {
        time_wheels_.push_back(time_wheel);
        return;
    }

    TimeWheelPtr prev_time_wheel = time_wheels_.back();
    prev_time_wheel->SetNextTimeWheel(time_wheel.get());
    time_wheel->SetPrevTimeWheel(prev_time_wheel.get());
    time_wheels_.push_back(time_wheel);
}

void TimeScheduler::Run() {
    while(true) {
        this_thread::sleep_for(MS(step_ms_));

        lock_guard<mutex> locker(mtx_);

        if(is_stop_) break;
        TimeWheelPtr least_time_wheel = GetLastTimeWheel();
        least_time_wheel->Tick();
        vector<TimerPtr> slot = move(least_time_wheel->MoveCurrentSlot());
        for(const TimerPtr& timer: slot) {
            if (timer->GetRepeated()) {
                timer->Update();
                GetFirstTimeWheel()->InsertTimer(timer);
                continue;
            }
            timer->Execute();
        }
    }
}

TimeWheelPtr TimeScheduler::GetFirstTimeWheel() {
    assert(time_wheels_.size() > 0);

    return time_wheels_.front();
}

TimeWheelPtr TimeScheduler::GetLastTimeWheel() {
    assert(time_wheels_.size() > 0);

    return time_wheels_.back();
}