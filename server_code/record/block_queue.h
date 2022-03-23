# ifndef BLOCK_QUEUE_H
# define BLOCK_QUEUE_H

# include <mutex>
# include <queue>
# include <assert.h>
# include <condition_variable>
# include <sys/time.h>

using namespace std;

template<class T>
class BlockQueue {
public:
    BlockQueue(int max_size = 1000);

    ~BlockQueue();

    void Close();

    int GetSize();

    int GetMaxSize();

    T GetFront();

    void Push(const T& item);

    bool Pop(T &item);

    bool Empty();

    bool Full();

    void Flush();
private:
    queue<T> que_;

    size_t max_size_;

    mutex mtx_;

    bool is_close_;

    condition_variable cv_consumer_, cv_producer_;
};

template<class T>
BlockQueue<T>::BlockQueue(int max_size) {
    assert(max_size > 0);
    max_size_ = max_size;
    is_close_ = false;
}

template<class T>
BlockQueue<T>::~BlockQueue() {
    Close();
}

template<class T>
void BlockQueue<T>::Close() {
    {
        lock_guard<mutex> locker(mtx_);
        queue<T> empty;
        swap(que_, empty);
        is_close_ = true;
    }

    cv_consumer_.notify_all();
    cv_producer_.notify_all();
}

template<class T>
int BlockQueue<T>::GetSize() {
    lock_guard<mutex> locker(mtx_);
    return que_.size();
}

template<class T>
int BlockQueue<T>::GetMaxSize() {
    lock_guard<mutex> locker(mtx_);
    return max_size_;
}

template<class T>
T BlockQueue<T>::GetFront() {
    lock_guard<mutex> locker(mtx_);
    return que_.front();
}

template<class T>
void BlockQueue<T>::Push(const T &item) {
    unique_lock<mutex> locker(mtx_);
    while(que_.size() >= max_size_) {
        cv_producer_.wait(locker);
    }
    que_.push(item);
    cv_consumer_.notify_one();
}

template<class T>
bool BlockQueue<T>::Pop(T &item) {
    unique_lock<mutex> locker(mtx_);
    while(que_.empty()) {
        cv_consumer_.wait(locker);
        if(is_close_) return false;
    }
    item = que_.front();
    que_.pop();
    cv_producer_.notify_one();
    return true;
}

template<class T>
bool BlockQueue<T>::Empty() {
    lock_guard<mutex> locker(mtx_);
    return que_.empty();
}

template<class T>
bool BlockQueue<T>::Full() {
    lock_guard<mutex> locker(mtx_);
    return que_.size() >= max_size_;
}

template<class T>
void BlockQueue<T>::Flush() {
    cv_consumer_.notify_one();
}

# endif 