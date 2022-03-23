# include "time_wheel.h"

using namespace std;

TimeWheel::TimeWheel(const string& name, int scales, int scales_unit_ms):
    name_(name),
    time_pos_(0),
    scales_(scales),
    step_ms_(scales_unit_ms),
    slots_(scales),
    next_(nullptr),
    prev_(nullptr) {}

int TimeWheel::GetCurrentTime() const{
    int current_time = time_pos_ * step_ms_;
    if(next_ != nullptr) {
        current_time += next_->GetCurrentTime();
    }

    return current_time;
}

void TimeWheel::InsertTimer(TimerPtr timer) {
    int next_time_wheel_time = 0;
    if(next_ != nullptr) {
        next_time_wheel_time = next_->GetCurrentTime();
    }

    int diff = timer->GetExecuteTime() + next_time_wheel_time - Timer::GetTimeNow();

    if(diff >= step_ms_) {
        int pos = (time_pos_ + diff / step_ms_) % scales_;
        slots_[pos].push_back(timer);
    }
    else if(next_ != nullptr) {
        next_->InsertTimer(timer);
    }
    else {
        slots_[time_pos_].push_back(timer);
    }
}

void TimeWheel::Tick() {
    time_pos_++;
    if(time_pos_ < scales_) return;

    time_pos_ %= scales_;
    if(prev_ != nullptr) {
        prev_->Tick();
        auto slot = move(prev_->MoveCurrentSlot());
        for(TimerPtr timer: slot) {
            InsertTimer(timer);
        }
    }
}

vector<TimerPtr> TimeWheel::MoveCurrentSlot() {
    vector<TimerPtr> slot;
    slot = move(slots_[time_pos_]);
    return slot;
}