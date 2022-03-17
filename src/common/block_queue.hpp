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
class BlockQueue {
public:
    using size_type = typename std::vector<T>::size_type;

public:
    BlockQueue(int cap) : m_maxCapacity(cap), m_queue(cap) {
        m_queue.shrink_to_fit();
    }
    ~BlockQueue(){}

    BlockQueue(const BlockQueue &) = delete;
    BlockQueue &operator = (const BlockQueue &) = delete;

public:
    // blocking api
    void put(const T t);
    T take();

    // non-blocking api
    bool offer(const T t);
    bool poll(T& t);
    bool offer(const T t, std::chrono::milliseconds& time);
    bool poll(T& t, std::chrono::milliseconds& time);

    bool empty() const{
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_sz_ == 0;
    }

    bool full() const{
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_sz_ >= m_maxCapacity;
    }

    size_type size(){
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_sz_;
    }

private:
    std::vector<T> m_queue;
    const int m_maxCapacity;
    size_t m_head_ = 0, m_tail_ = 0;
    int m_sz_ = 0;
    mutable std::mutex m_mutex;
    std::condition_variable m_cond_empty;
    std::condition_variable m_cond_full;
};

template <class T>
void BlockQueue<T>::put(const T t){
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cond_full.wait(lock, [this]{ return m_sz_ < m_maxCapacity; });
    m_queue[m_tail_] = t;
    m_tail_ = (m_tail_ + 1) % m_maxCapacity;
    m_sz_++;
    m_cond_empty.notify_all();
}

template <class T>
T BlockQueue<T>::take(){
    std::unique_lock<std::mutex> lock(m_mutex);
    // take必须判断队列为空
    m_cond_empty.wait(lock, [&](){return m_sz_ > 0;});
    auto res = m_queue[m_head_];
    m_head_ = (m_head_ + 1) % m_maxCapacity;
    m_sz_--;
    m_cond_full.notify_all();
    return res;
}

template <class T>
bool BlockQueue<T>::offer(const T t){
    std::lock_guard<std::mutex> lock(m_mutex);
    if(m_sz_ >= m_maxCapacity){
        return false;
    }
    m_queue[m_tail_] = t;
    m_tail_ = (m_tail_ + 1) % m_maxCapacity;
    m_sz_++;
    m_cond_empty.notify_all();
    return true;
}

template <class T>
bool BlockQueue<T>::poll(T& t){
    std::lock_guard<std::mutex> lock(m_mutex);
    if(m_sz_ == 0){
        return false;
    }
    t = m_queue[m_head_];
    m_head_ = (m_head_ + 1) % m_maxCapacity;
    m_sz_--;
    m_cond_full.notify_all();
    return true;
}

template <class T>
bool BlockQueue<T>::offer(const T t, std::chrono::milliseconds& time){
    std::lock_guard<std::mutex> lock(m_mutex);
    bool result = m_cond_full.wait(lock, time, 
                                [&]{ return m_sz_ < m_maxCapacity; });
    if(!result){
        return false;
    }
    m_queue[m_tail_] = t;
    m_tail_ = (m_tail_ + 1) % m_maxCapacity;
    m_sz_++;
    m_cond_empty.notify_all();
    return true;
}

template <class T>
bool BlockQueue<T>::poll(T& t, std::chrono::milliseconds& time){
    std::lock_guard<std::mutex> lock(m_mutex);
    bool result = m_cond_empty.wait_for(lock, time, 
                                   [&] {return m_sz_ > 0;});
    if(!result){
        return false;
    }
    t = m_queue[m_head_];
    m_head_ = (m_head_ + 1) % m_maxCapacity;
    m_sz_--;
    m_cond_full.notify_all();
    return true;
}



template<class T>
class BlockQueueSTL {
public:
    using size_type = typename std::deque<T>::size_type;

public:
    BlockQueueSTL(const int cap = -1) : m_maxCapacity(cap){}
    ~BlockQueueSTL(){}

    BlockQueueSTL(const BlockQueueSTL &) = delete;
    BlockQueueSTL &operator = (const BlockQueueSTL &) = delete;

public:
    // blocking api
    void put(const T t);
    T take();

    // non-blocking api
    bool offer(const T t);
    bool poll(T& t);
    bool offer(const T t, std::chrono::milliseconds& time);
    bool poll(T& t, std::chrono::milliseconds& time);

    bool empty() const{
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    bool full() const{
        if(-1 == m_maxCapacity)
            return false;
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size() >= m_maxCapacity;
    }

    size_type size(){
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

private:
    std::deque<T> m_queue;
    const int m_maxCapacity;
    mutable std::mutex m_mutex;
    std::condition_variable m_cond_empty;
    std::condition_variable m_cond_full;
};

template <class T>
void BlockQueueSTL<T>::put(const T t){
    std::unique_lock<std::mutex> lock(m_mutex);
    if(m_maxCapacity != -1){
        m_cond_full.wait(lock, [this]{ return m_queue.size() < m_maxCapacity; });
    }
    m_queue.push_back(t);
    m_cond_empty.notify_all();
}

template <class T>
T BlockQueueSTL<T>::take(){
    std::unique_lock<std::mutex> lock(m_mutex);
    // take必须判断队列为空
    m_cond_empty.wait(lock, [&](){return !m_queue.empty();});
    auto res = m_queue.front();
    m_queue.pop_front();
    m_cond_full.notify_all();
    return res;
}

template <class T>
bool BlockQueueSTL<T>::offer(const T t){
    std::lock_guard<std::mutex> lock(m_mutex);
    if(m_maxCapacity != -1 && m_queue.size() >= m_maxCapacity){
        return false;
    }
    m_queue.push_back(t);
    m_cond_empty.notify_all();
    return true;
}

template <class T>
bool BlockQueueSTL<T>::poll(T& t){
    std::lock_guard<std::mutex> lock(m_mutex);
    if(m_queue.empty()){
        return false;
    }
    t = m_queue.front();
    m_queue.pop_front();
    m_cond_full.notify_all();
    return true;
}

template <class T>
bool BlockQueueSTL<T>::offer(const T t, std::chrono::milliseconds& time){
    std::lock_guard<std::mutex> lock(m_mutex);
    if(m_maxCapacity != -1){
        bool result = m_cond_full.wait(lock, time, 
                                   [&]{ return m_queue.size() < m_maxCapacity; });
        if(!result){
            return false;
        }
    }
    m_queue.push_back(t);
    m_cond_empty.notify_all();
    return true;
}

template <class T>
bool BlockQueueSTL<T>::poll(T& t, std::chrono::milliseconds& time){
    std::lock_guard<std::mutex> lock(m_mutex);
    bool result = m_cond_empty.wait_for(lock, time, 
                                   [&] {return !m_queue.empty();});
    if(!result){
        return false;
    }
    t = m_queue.front();
    m_queue.pop_front();
    m_cond_full.notify_all();
    return true;
}

static void testBlockQueue(){
    auto produce = [](BlockQueue<int> &q) {
        const int num = 9;
        for(int i = 0; i < num; ++i){
            //q.offer(i);  // 只打印  0   1
            q.put(i);
        }   
    };
    auto consume = [](BlockQueue<int> &q) {
        while(false == q.empty()){
            int tmp = q.take();
            std::cout << tmp << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    };
    BlockQueue<int> iqueue(2);
    std::thread t1(produce, std::ref(iqueue));
    std::thread t2(consume, std::ref(iqueue));
    std::thread t3(consume, std::ref(iqueue));
    t1.join();
    t2.join();
    t3.join();
}

}