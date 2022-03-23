# ifndef THREADPOOL_H
# define THREADPOOL_H

# include <mutex>
# include <condition_variable>
# include <queue>
# include <functional>
# include <thread>
# include <assert.h>

using namespace std;

typedef function<void()> ThreadTask;

class ThreadPool {
public:
    ThreadPool(int thread_num = 8) {
        assert(thread_num > 0);
        for(int i = 0; i < thread_num; i++) {
            thread(
                [=] {
                    unique_lock<mutex> locker(mtx_);
                    while(true) {
                        if(!task_.empty()) {
                            auto task = move(task_.front());
                            task_.pop();
                            locker.unlock();
                            task();
                            locker.lock();
                        }
                        else if(is_stop_) break;
                        else cv_.wait(locker);
                    }
                }
            ).detach();
        }
    }

    ~ThreadPool() {
        {
            lock_guard<mutex> locker(mtx_);
            is_stop_ = true;
        }
        
        cv_.notify_all();

    }

    void EnQueue(ThreadTask&& task) {
        lock_guard<mutex> locker(mtx_);
        task_.push(forward<ThreadTask> (task));

        cv_.notify_one();
    }

private:
    mutex mtx_;
    condition_variable cv_;
    bool is_stop_;
    queue<ThreadTask> task_;
};

// ThreadPool::ThreadPool(int thread_num) {
//     assert(thread_num > 0);
// }

# endif