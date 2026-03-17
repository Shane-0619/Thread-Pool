#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <unordered_map>


//Any类型：可以接受任意数据类型
class Any
{
public:
    Any() = default;
    ~Any() = default;
    Any(const Any&) = delete;
    Any& operator=(const Any&) = delete;
    
    Any(Any&&) = default;
    Any& operator=(Any&&) = default;

    template<typename T>
    Any(T data) : base_(std::make_unique<Derive<T>>(data))
    {}

    template<typename T>
    T cast()
    {
        Derive<T> *pd = dynamic_cast<Derive<T>*>(base_.get());
        if(pd == nullptr)
        {
            throw "Data type doesn't match!";
        }
        return pd->data_;
    }
    
private:
    class Base
    {
    public:
        virtual ~Base() = default;
    };
    template<typename T>
    class Derive : public Base
    {
    public:
        Derive(T data) : data_(data)
        {}
        T data_;
    };

    std::unique_ptr<Base> base_;
};

class Result;
//任务抽象基类
//用户自定义任务类型，从task继承，重写run方法，实现自定义任务处理
class Task
{
public:
    Task();
    ~Task() = default;
    virtual Any run() = 0;
    void exec();
    void setResult(Result* result);
private:
    Result* result_;
};

//实现信号量类
class Semaphore
{
public:
    Semaphore(size_t limit = 0) : resLimit_(limit)
    {}
    ~Semaphore() = default;

    void wait();
    void post();

private:
    size_t resLimit_;
    std::mutex mtx_;
    std::condition_variable cond_;
};

//获取任务返回值
class Result
{
public:
    Result(std::shared_ptr<Task> task, bool isValid = true);

    // Result 可以移动，但不能复制
    Result(Result&& other) noexcept;
    Result& operator=(Result&& other) noexcept;

    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;

    ~Result();

    void setVal(Any val);

    Any get();
private:
    Any val_;
    std::shared_ptr<Task> task_;
    std::atomic_bool isValid_;
    Semaphore sem_;
};

//线程池支持的模式
enum class PoolMode
{
    MOD_FIXED,      //固定数量的线程
    MOD_CACHED,     //线程数量可动态增长
};

//线程类型
class Thread
{
public:
    //线程函数对象类型
    using ThreadFunc = std::function<void(size_t)>;
    
    //线程构造
    Thread(ThreadFunc func);

    //线程析构
    ~Thread();

    //线程启动
    void start();

    void join();

    //获取线程Id
    size_t getId() const;
private:
    ThreadFunc func_;
    static size_t generateId_;
    size_t threadId_;
    std::thread thread_;
};

//线程池类型
class ThreadPool
{
public:
    //线程池构造
    ThreadPool();

    //线程池析构
    ~ThreadPool();

    //设置线程池的工作模式
    void setMode(PoolMode mode);

    //设置任务队列上限阈值
    void setTaskQueMaxThreshHold(size_t threshhold);

    //设置cached模式下线程阈值
    void setThreadSizeThreshHold(size_t threshhold);

    //向线程池提交任务
    Result submitTask(std::shared_ptr<Task> sp);

    //开启线程池
    void start(size_t initThreadSize = 4);

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
private:
    //线程函数
    void threadFunc(size_t threadId);

    //检查当前线程池的启动状态
    bool checkRunningState();
private:
    //std::vector<std::unique_ptr<Thread>> threads_;  //线程列表
    std::unordered_map<size_t, std::unique_ptr<Thread>> threads_;   //线程列表

    size_t initThreadSize_; //初始的线程数量
    std::atomic_int idleThreadSize_;    //记录空闲线程的数量
    int threadSizeThreshHold_;  //线程数量上限阈值
    std::atomic_int curThreadSize_; //当前线程的数量

    std::queue<std::shared_ptr<Task>> taskQue_; //任务队列
    std::atomic_uint taskSize_;  //任务数量
    size_t taskQueMaxThreshHold_;   //任务队列的上限阈值

    std::mutex taskQueMtx_; //保证任务队列的线程安全
    std::condition_variable notFull_;   //任务队列不满
    std::condition_variable notEmpty_;   //任务队列不空

    PoolMode poolMode_; //当前线程池的工作模式
    std::atomic_bool isPoolRunning_;    //当前线程池的启动状态

};


#endif