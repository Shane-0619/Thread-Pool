#include "threadpool.h"
#include <functional>
#include <thread>
#include <memory>
#include <iostream>
#include <mutex>

const size_t TASK_MAX_THRESHHOLD = INT32_MAX;
const size_t Thread_MAX_THRESHOLD = 5;
const size_t THREAD_MAX_IDLE_TIME = 10; //单位：秒

/*                                              Task类实现                                  */
//Task构造函数
Task::Task()
    : result_(nullptr)
{}

//Task执行函数
void Task::exec()
{
    if(result_!=nullptr)
    {
        result_->setVal(run());
    }
    else
    {
        run();
    }
}

//Task内部关联Result（相互关联）
void Task::setResult(Result* result)
{
    result_ = result;
}

/*                                              ThreadPool类实现                              */
//线程池构造
ThreadPool::ThreadPool()
    : initThreadSize_(0)
    , taskSize_(0)
    , taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)
    , poolMode_(PoolMode::MOD_FIXED)
    , isPoolRunning_(false)
    , idleThreadSize_(0)
    , threadSizeThreshHold_(Thread_MAX_THRESHOLD)
    , curThreadSize_(0)
{}

//线程池析构
ThreadPool::~ThreadPool()
{
    isPoolRunning_ = false;
    
    {
        std::unique_lock<std::mutex> lock(taskQueMtx_);
        notEmpty_.notify_all();
    }

    for(auto &pair : threads_)
    {
        pair.second->join();
    }
}

//设置线程池的工作模式
void ThreadPool::setMode(PoolMode mode)
{
    if(checkRunningState())
        return;
    poolMode_ = mode;
}

//设置任务队列上限阈值
void ThreadPool::setTaskQueMaxThreshHold(size_t threshhold)
{
    taskQueMaxThreshHold_ = threshhold;
}

//设置cached模式下线程阈值
void ThreadPool::setThreadSizeThreshHold(size_t threshhold)
{
    if(checkRunningState())
        return;
    if(poolMode_ == PoolMode::MOD_CACHED)
        threadSizeThreshHold_ = threshhold;
}

//向线程池提交任务
Result ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
    std::unique_lock<std::mutex> lock(taskQueMtx_);

    if(!notFull_.wait_for(lock, std::chrono::seconds(1),
        [&]()->bool{ return taskQue_.size() < taskQueMaxThreshHold_;}))
    {
        std::cerr << "The task queue is full, submit task fail." << std::endl;
        return Result(sp, false);
    }

    taskQue_.emplace(sp);
    taskSize_++;

    notEmpty_.notify_all();

    //cached模式：任务处理比较紧急，适合小而快的任务，根据任务数量和空闲线程的数量，判断是否需要添加线程
    if(poolMode_ == PoolMode::MOD_CACHED
        && taskSize_ > idleThreadSize_
        && curThreadSize_ < threadSizeThreshHold_)
    {
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
        size_t threadId = ptr->getId();
        threads_.emplace(threadId, std::move(ptr));

        threads_[threadId]->start();
        curThreadSize_++;
        idleThreadSize_++;
        std::cout << "create a new thread: " << threadId << std::endl;
    }

    return Result(sp);
}

//开启线程池
void ThreadPool::start(size_t initThreadSize)
{
    initThreadSize_ = initThreadSize;
    curThreadSize_ = initThreadSize;
    isPoolRunning_ = true;
    
    for(int i = 0; i < initThreadSize_; i++)
    {
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
        size_t threadId = ptr->getId();
        threads_.emplace(threadId, std::move(ptr));
    }

    for(auto &pair : threads_)
    {
        pair.second->start();
        idleThreadSize_++;
    }

}


//线程函数
void ThreadPool::threadFunc(size_t threadId)
{    
    auto lastTime = std::chrono::high_resolution_clock().now();
    for(;;)
    {   
        std::shared_ptr<Task> task; 
        {
            std::unique_lock<std::mutex> lock(taskQueMtx_);

            std::cout << "tid: " << std::this_thread::get_id() 
                << "尝试获取任务......" << std::endl;
            while(taskQue_.size()==0)
            {
                if(!isPoolRunning_)
                {
                    std::cout << "threadid: " << std::this_thread::get_id() << " exit" << std::endl;
                    return;
                }                
                //cached模式下回收多余的线程(空闲时间超过60s)
                if(poolMode_ == PoolMode::MOD_CACHED)
                {
                    if(std::cv_status::timeout == 
                        notEmpty_.wait_for(lock, std::chrono::seconds(1)))
                    {
                        auto now = std::chrono::high_resolution_clock().now();
                        auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
                        if(dur.count() >= THREAD_MAX_IDLE_TIME
                            && curThreadSize_ > initThreadSize_)
                        {
                            curThreadSize_--;
                            idleThreadSize_--;
                            std::cout << "threadid: " << std::this_thread::get_id() << " exit" << std::endl;
                            return;
                        }
                    }
                }
                else
                {
                    notEmpty_.wait(lock);
                }
            }
            idleThreadSize_--;
            std::cout << "tid: " << std::this_thread::get_id()
                << "获取任务成功！" << std::endl;

            task = taskQue_.front();
            taskQue_.pop();
            taskSize_--;

            if(taskQue_.size() > 0)
            {
                notEmpty_.notify_all();
            }
            notFull_.notify_all();
        }
        
        if(task != nullptr)
        {
            task->exec();
        }
        lastTime = std::chrono::high_resolution_clock().now();
        idleThreadSize_++;
    }
}

//检查当前线程池的启动状态
bool ThreadPool::checkRunningState()
{
    return isPoolRunning_;
}
/*                                              Semaphore类实现                                      */
void Semaphore::wait()
{
    std::unique_lock<std::mutex> lock(mtx_);
    cond_.wait(lock, [&]()->bool{ return resLimit_ > 0;});
    resLimit_--;
}

void Semaphore::post()
{
    std::unique_lock<std::mutex> lock(mtx_);
    resLimit_++;
    cond_.notify_all();
}

/*                                              Thread类实现                                          */
size_t Thread::generateId_ = 0;
//线程构造
Thread::Thread(ThreadFunc func)
    : func_(func)
    , threadId_(generateId_++)
{}

//线程析构
Thread::~Thread(){}

//线程启动
void Thread::start()
{
    if (!func_) {
        std::cerr << "Thread::start: func_ is empty!" << std::endl;
        return;
    }
    
    thread_ = std::thread(func_, threadId_);
}

void Thread::join()
{
    if(thread_.joinable())
    {
        thread_.join();
    }
}

//获取线程Id
size_t Thread::getId() const
{
    return threadId_;
}

/*                                              Result类相关实现                                        */  
//Result构造函数
Result::Result(std::shared_ptr<Task> task, bool isValid)
    : task_(task)
    , isValid_(isValid)
{
    if(task_)
    {
        task_->setResult(this);
    }
}

//Result移动构造（需要更新Task中的Result指针）
Result::Result(Result&& other) noexcept
    : val_(std::move(other.val_))
    , task_(std::move(other.task_))
    , isValid_(other.isValid_.load())
    , sem_()
{
    if(task_)
    {
        task_->setResult(this);
    }
}

Result& Result::operator=(Result&& other) noexcept
{
    if(this != &other)
    {
        val_ = std::move(other.val_);
        task_ = std::move(other.task_);
        isValid_ = other.isValid_.load();
        if(task_)
        {
            task_->setResult(this);
        }
    }
    return *this;
}

//设置Result::val_的值
void Result::setVal(Any val)
{
    this->val_ = std::move(val);
    sem_.post();
}

//返回Result::val_
Any Result::get()
{
    if(!isValid_)
    {
        return "";
    }
    sem_.wait();
    return std::move(val_);
}

Result::~Result()
{
    if(isValid_ && task_ != nullptr)
    {
        task_->setResult(nullptr);
    }
}