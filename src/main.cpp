#include "threadpool.h"
#include <thread>
#include <chrono>
#include <iostream>

class Mytask : public Task
{
public:
    void run()
    {
        std::cout << "tid: " << std::this_thread::get_id()
            << " begin!" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "tid: " << std::this_thread::get_id()
            << " end!" << std::endl;
    }
};

int main()
{
    ThreadPool tp;
    tp.start(4);

    tp.submitTask(std::make_shared<Mytask>());
    tp.submitTask(std::make_shared<Mytask>());
    tp.submitTask(std::make_shared<Mytask>());
    tp.submitTask(std::make_shared<Mytask>());
    tp.submitTask(std::make_shared<Mytask>());
    tp.submitTask(std::make_shared<Mytask>());
    tp.submitTask(std::make_shared<Mytask>());
    tp.submitTask(std::make_shared<Mytask>());
    tp.submitTask(std::make_shared<Mytask>());

    // 等待任务完成
    std::this_thread::sleep_for(std::chrono::seconds(5));

    return 0;
}