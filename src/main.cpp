#include "threadpool.h"
#include <thread>
#include <chrono>
#include <iostream>

using uLong = unsigned long long;
class Mytask : public Task
{
public:
    Mytask(int begin, int end)
        : begin_(begin)
        , end_(end)
    {}
    Any run()
    {
        std::cout << "tid: " << std::this_thread::get_id()
            << " begin!" << std::endl;
        uLong sum = 0;
        for(uLong i = begin_; i<=end_; i++)
        {
            sum+=i;
        }
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::cout << "tid: " << std::this_thread::get_id()
            << " end!" << std::endl;
        return sum;
    }
private:
    int begin_;
    int end_;
};

int main()
{
    ThreadPool pool;
    pool.setMode(PoolMode::MOD_CACHED);
    pool.start(2);

    Result res1 = pool.submitTask(std::make_shared<Mytask>(1, 100000000));
    Result res2 = pool.submitTask(std::make_shared<Mytask>(100000001, 200000000));
    Result res3 = pool.submitTask(std::make_shared<Mytask>(200000001, 300000000));

    pool.submitTask(std::make_shared<Mytask>(200000001, 300000000));
    pool.submitTask(std::make_shared<Mytask>(200000001, 300000000));
    pool.submitTask(std::make_shared<Mytask>(200000001, 300000000));

    // Result res4 = pool.submitTask(std::make_shared<Mytask>(1, 100000000));
    // Result res5 = pool.submitTask(std::make_shared<Mytask>(100000001, 200000000));
    // Result res6 = pool.submitTask(std::make_shared<Mytask>(200000001, 300000000));

    uLong sum1 = res1.get().cast<uLong>();
    uLong sum2 = res2.get().cast<uLong>();
    uLong sum3 = res3.get().cast<uLong>();

    std::cout << sum1 + sum2 + sum3 << std::endl;
    std::cout << "任务完成，按Enter键让程序等待并观察线程回收..." << std::endl;
    std::cin.get(); 
    std::cout << "程序结束。" << std::endl;
    return 0;

}