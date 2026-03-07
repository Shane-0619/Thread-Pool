#include "threadpool.h"
#include <thread>
#include <chrono>

int main()
{
    ThreadPool tp;
    tp.start(4);

    //延长主线程，保证得到子线程测试结果
    std::this_thread::sleep_for(std::chrono::seconds(5));
    return 0;
}