#pragma once

#include <queue>
#include <pthread.h>

#define CAPACITY 10000
template <class T>

class MsgQueue
{
public:
    MsgQueue()
    {
        capacity_ = CAPACITY;
        pthread_mutex_init(&lock_vec_, NULL);
        pthread_cond_init(&cons_cond_, NULL);
        pthread_cond_init(&prod_cond_, NULL);
    }

    ~MsgQueue()
    {
        pthread_mutex_destroy(&lock_vec_);
        pthread_cond_destroy(&cons_cond_);
        pthread_cond_destroy(&prod_cond_);
    }

    void Push(const T& msg)
    {
        pthread_mutex_lock(&lock_vec_);
        while(vec_.size() >= capacity_)
        {
            //大于最大内存，阻塞等待
            pthread_cond_wait(&prod_cond_, &lock_vec_);
        }
        vec_.push(msg);
        pthread_mutex_unlock(&lock_vec_);

        //完全写入，通知消费者
        pthread_cond_signal(&cons_cond_);
    }

    void Pop(T* msg)
    {
        pthread_mutex_lock(&lock_vec_);
        while(vec_.empty())
        {
            pthread_cond_wait(&cons_cond_, &lock_vec_);
        }
        *msg = vec_.front();
        vec_.pop();
        pthread_mutex_unlock(&lock_vec_);

        pthread_cond_signal(&prod_cond_);
    }


private:
    std::queue<T> vec_;
    size_t capacity_;

    pthread_mutex_t lock_vec_;
    pthread_cond_t cons_cond_;//消费者条件变量
    pthread_cond_t prod_cond_;//生产者条件变量
};
