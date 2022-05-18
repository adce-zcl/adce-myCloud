#ifndef THREAD_POOL_HPP_
#define THREAD_POOL_HPP_
#include "thread_queue.hpp"
#include <thread>
#include <functional>
#include <vector>
#include <atomic>
class join_threads
{
private:
    // 对线程池的引用
    std::vector<std::thread> &threads;
public:
    // 禁止隐式类型转换，构造函数
    explicit join_threads(std::vector<std::thread> &threads_) : threads(threads_){}
    // 析构的时候对所有线程加入，等待所有线程结束
    ~join_threads()
    {
        for (unsigned long i = 0; i < threads.size(); ++i)
        {
            if (threads.at(i).joinable())
            {
                threads.at(i).join();
            }
        }
    }
};

class thread_pool
{
private:
    std::atomic_bool done;
    // 任务池实体 - 任务只接受 形如 void fun(std::string) 类型的函数
    thread_safe_queue<std::function<void(std::string other)>> work_queue;
    std::queue<std::string> paras;
    // 线程池实体
    std::vector<std::thread> threads;
    // 只需要定义一个joiner就好了，析构的时候自动执行join
    join_threads joiner;

    // 线程池里的线程都运行这个函数
    void worker_thread()
    {
        // 如果线程池没有关闭
        while (!done)
        {
            std::function<void(std::string other)> task;
            if (work_queue.try_pop(task))
            {
                std::string par = paras.front();
                paras.pop();
                task(par);
            }
            else
            {
                // 出让调度器
                std::this_thread::yield();
            }
        }
    }
public:
    thread_pool() : done(false), joiner(threads)
    {
        // 尽量使用多一些线程处理任务，太少的话可能会阻塞程序
        const unsigned int thread_count = std::thread::hardware_concurrency() + 10;
        try
        {
            for (unsigned int i = 0; i < thread_count; ++i)
            {
                // 新建thread_count数量的线程
                threads.push_back(std::thread(&thread_pool::worker_thread, this));
            }
        }
        catch(...)
        {
            done = false;
            throw;
        }
    }
    // 向任务池里推送任务
    template<typename FunctionType>
    void submit(FunctionType f, std::string para)
    {
        work_queue.push(std::function<void(std::string)>(f));
        paras.push(para);
    }

    ~thread_pool()
    {
        done = false;
    }
};
#endif