#include "threadpool.h"
#include <functional>
#include <thread>
#include <iostream>

const size_t TASK_MAX_THRESHHOLD = 1024;

//线程池构造
ThreadPool::ThreadPool()
    : initThreadSize_(0)
    , taskSize_(0)
    , taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)
    , poolMode_(PoolMode::MOD_FIXED)
{}

//线程池析构
ThreadPool::~ThreadPool()
{}

//设置线程池的工作模式
void ThreadPool::setMode(PoolMode mode)
{
    poolMode_ = mode;
}

//设置任务队列上限阈值
void ThreadPool::setTaskQueMaxThreshHold(size_t threshhold)
{
    taskQueMaxThreshHold_ = threshhold;
}

//向线程池提交任务
void ThreadPool::submitTask(std::shared_ptr<Task> sp)
{

}

//开启线程池
void ThreadPool::start(size_t initThreadSize)
{
    //记录初始线程数量
    initThreadSize_ = initThreadSize;

    //创建线程对象
    for(int i = 0; i < initThreadSize_; i++)
    {
        threads_.emplace_back(new Thread(std::bind(&ThreadPool::threadFunc, this)));
    }

    //启动所有线程
    for(int i = 0; i < initThreadSize_; i++)
    {
        threads_[i]->start();
    }

}

//线程函数
void ThreadPool::threadFunc()
{
    std::cout << "begin threadFunc: "
        << std::this_thread::get_id() 
        << std::endl;
    std::cout << "end threadFunc: "
        << std::this_thread::get_id()
        << std::endl;
}

////////////////    线程方法实现

//线程构造
Thread::Thread(ThreadFunc func)
    : func_(func)
{}

//线程析构
Thread::~Thread(){}

//启动线程
void Thread::start()
{
    //创建线程执行线程函数
    std::thread t(func_);
    t.detach();
}