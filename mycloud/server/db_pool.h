#pragma once
#include <mysql++.h>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <iostream>
// 打印消息
void show_info(std::string info);


class DB_POOL
{
    // 删除的函数
    DB_POOL(const DB_POOL &other) = delete;
    DB_POOL(DB_POOL &&other) = delete;
    DB_POOL& operator=(const DB_POOL &other) = delete;
    DB_POOL &operator=(DB_POOL &&other) = delete;

private:
    unsigned int maxSize;           // 数据库连接池的最大数量
    std::string host_read;          // 从库的IP地址
    std::string host_wirte;         // 主库的IP地址
    std::string name;               // 用户名
    std::string passwd;             // 密码
    unsigned int port;              // 端口
    std::string db;                 // 数据库名字
    mutable std::mutex mut;         // 互斥量
    std::condition_variable cond;   // 条件变量
    std::queue<mysqlpp::TCPConnection*> read_pool;   // 从库的连接池
    std::queue<mysqlpp::TCPConnection*> write_pool;   // 主库的连接池

    static unsigned int count;          // 引用计数
    static DB_POOL *db_pool;            // 数据库连接池指针
    DB_POOL();
    // 装在配置文件
    bool db_get_info();

public:
    // 初始化
    static DB_POOL *db_pool_init();
    // 从连接池里获取一个连接，flag代表是需要读池里的连接池，还是写池里连接池
    mysqlpp::TCPConnection* get_connect(int flag);
    // 把链接归还到指定的池子里
    void put_connect(mysqlpp::TCPConnection *connect, int flag);
    // 析构函数
    ~DB_POOL();
};
