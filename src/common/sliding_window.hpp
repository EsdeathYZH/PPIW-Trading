#pragma once
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>
#include <chrono>
#include <thread>

namespace ubiquant {

template<class T>
class SlidingWindow {
public:
    using size_type = typename std::vector<T>::size_type;

public:
    SlidingWindow() : capacity_(0) {}
    SlidingWindow(const int capacity)  
        : data_(capacity), avaliable_(capacity, false), capacity_(capacity) {}
    ~SlidingWindow(){}

    SlidingWindow(const SlidingWindow &) = delete;
    SlidingWindow &operator = (const SlidingWindow &) = delete;
    SlidingWindow(SlidingWindow &&) = default;
    SlidingWindow &operator = (SlidingWindow &&) = default;

    inline int get_capacity() { return capacity_; }

public:
    // blocking api
    void put(const T t, size_t idx);
    T get();

    // non-blocking api
    // NOTICE: we have no offer function because every put operation will be success
    bool poll(T& t);
    bool poll(T& t, std::chrono::milliseconds& time);

private:
    std::vector<T> data_;
    std::vector<bool> avaliable_;
    const int capacity_;

    size_t head_ = 0;
    mutable std::mutex m_mutex;
    std::condition_variable m_cond_avail;
};

template <class T>
void SlidingWindow<T>::put(const T t, size_t idx){
    std::unique_lock<std::mutex> lock(m_mutex);
    ASSERT(!avaliable_[idx]);
    data_[idx] = t;
    avaliable_[idx] = true;
    if(idx == head_) {
        m_cond_avail.notify_all();
    }
}

template <class T>
T SlidingWindow<T>::get(){
    std::unique_lock<std::mutex> lock(m_mutex);
    // get必须判断head位置avaliable
    m_cond_avail.wait(lock, [&](){return avaliable_[head_];});
    auto res = data_[head_];
    avaliable_[head_] = false;
    head_ = (head_ + 1) % capacity_;
    return res;
}

template <class T>
bool SlidingWindow<T>::poll(T& t){
    std::lock_guard<std::mutex> lock(m_mutex);
    if(!avaliable_[head_]){
        return false;
    }
    t = data_[head_];
    avaliable_[head_] = false;
    head_ = (head_ + 1) % capacity_;
    return true;
}

template <class T>
bool SlidingWindow<T>::poll(T& t, std::chrono::milliseconds& time){
    std::lock_guard<std::mutex> lock(m_mutex);
    bool result = m_cond_avail.wait_for(lock, time, [&] {return avaliable_[head_];});
    if(!result){
        return false;
    }
    t = data_[head_];
    avaliable_[head_] = false;
    head_ = (head_ + 1) % capacity_;
    return true;
}

static void testSlidingWindow(){
    auto produce = [](SlidingWindow<int> &q) {};
    auto consume = [](SlidingWindow<int> &q) {};
    SlidingWindow<int> window(2);
    std::thread t1(produce, std::ref(window));
    std::thread t2(consume, std::ref(window));
    std::thread t3(consume, std::ref(window));
    t1.join();
    t2.join();
    t3.join();
}

}