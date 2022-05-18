#ifndef THREAD_QUEUE_HPP_
#define THREAD_QUEUE_HPP_
// 使用条件变量的线程安全队列的完整类定义
// 包含shared_ptr实例
#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>
#include <memory>
template <typename T>
class thread_safe_queue
{
private:
    // 队列，存放共享指针——直接存放数据容易异常
    std::queue<std::shared_ptr<T>> data_queue;
    // 互斥量
    mutable std::mutex mut;
    // 条件变量，用于通知队列是否有任务
    std::condition_variable cond;
public:
    // 删除函数，移动函数也应该删除
    thread_safe_queue(const thread_safe_queue &&other) = delete;
    thread_safe_queue(const thread_safe_queue &other) = delete;
    thread_safe_queue& operator=(const thread_safe_queue &other) = delete;
    thread_safe_queue &operator=(const thread_safe_queue &&other) = delete;
    // 默认构造函数
    thread_safe_queue(){}

    bool empty() const
    {
        // 上锁，防止查看队列是否为空的时候有别的线程操作队列，使得队列状态发生变化
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.empty();
    }

    void push(T new_value)
    {
        // 创建智能指针不需要上锁，我们只尽量锁最小粒度
        std::shared_ptr<T> data(std::make_shared<T>(std::move(new_value)));
        // 上锁
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(data);
        // 尽量不要使用notify_all，会有惊群效应
        cond.notify_one();  
    }

    // 阻塞版本的弹出任务
    void wait_and_pop(T &value)
    {
        // 这里必须用互斥锁，因为互斥锁可以人为解锁
        // 下面代码没有人为解锁？
        std::unique_lock<std::mutex> lk(mut);
        // 阻塞等待队列不为空
        // 条件变量加锁等待，不满足条件解锁阻塞，既然需要解锁，就必须用unique_lock
        cond.wait(lk,[this](){return !data_queue.empty();});    
        // 使用移动，减少拷贝
        value = std::move(*data_queue.front());
        // 对被移动的元素，只能执行删除操作
        data_queue.pop();
        // lk.unlock(); 析构自动解锁
    }

    // 函数重载
    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> lk(mut);
        cond.wait(lk,[this](){return !data_queue.empty();});
        std::shared_ptr<T> res = data_queue.front();
        data_queue.pop();
        return res;
    }

    // 非阻塞版本的弹出任务，有就弹出，没有就返回
    // 这是双返回值函数，既返回bool又填充数据
    bool try_pop(T &value)
    {
        std::lock_guard<std::mutex> lk(mut);
        if(data_queue.empty())
        {
            return false;
        }
        value = std::move(*data_queue.front());
        data_queue.pop();
        return true;
    }

    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lk(mut);
        if(data_queue.empty())
        {
            return std::make_shared<T>();
        }
        std::shared_ptr<T> res = data_queue.front();
        data_queue.pop();
        return res;
    }
};
#endif