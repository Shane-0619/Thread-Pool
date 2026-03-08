#include "threadpool.h"
#include <functional>
#include <thread>
#include <memory>
#include <iostream>
#include <mutex>

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
    std::unique_lock<std::mutex> lock(taskQueMtx_);

    if(!notFull_.wait_for(lock, std::chrono::seconds(1),
        [&]()->bool{ return taskQue_.size() < TASK_MAX_THRESHHOLD;}))
    {
        std::cerr << "The task queue is full, submit task fail." << std::endl;
    }

    taskQue_.emplace(sp);
    taskSize_++;

    notEmpty_.notify_all();
}

//开启线程池
void ThreadPool::start(size_t initThreadSize)
{
    //记录初始线程数量
    initThreadSize_ = initThreadSize;

    //创建线程对象
    for(int i = 0; i < initThreadSize_; i++)
    {
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this));
        threads_.emplace_back(std::move(ptr));
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
    // std::cout << "begin threadFunc: "
    //     << std::this_thread::get_id() 
    //     << std::endl;
    // std::cout << "end threadFunc: "
    //     << std::this_thread::get_id()
    //     << std::endl;
    
    
    for(;;)
    {   
        std::shared_ptr<Task> task; 
        {
            std::unique_lock<std::mutex> lock(taskQueMtx_);

            std::cout << "tid: " << std::this_thread::get_id() 
                << "尝试获取任务......" << std::endl;

            notEmpty_.wait(lock, [&]()->bool{ return taskQue_.size() > 0; });

            task = taskQue_.front();
            taskQue_.pop();
            taskSize_--;

            std::cout << "tid: " << std::this_thread::get_id()
                << "获取任务成功！" << std::endl;

            if(taskQue_.size() > 0)
            {
                notEmpty_.notify_all();
            }
            notFull_.notify_all();
        }
        
        if(task != nullptr)
        {
            task->run();
        }
    }
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