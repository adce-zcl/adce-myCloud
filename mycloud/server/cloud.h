#ifndef CLOUD_H_
#define CLOUD_H_
#include "thread_pool.hpp"
#include "db_pool.h"
#include <iostream>
#include <string>
#define BUFSIZE (1500 - 20 - 20)
#define PORT 1226
// 报文的类型
enum
{
    TYPE_CON = 0,   // 连接请求
    TYPE_COM = 1,   // 计算请求
    TYPE_SHOW = 2,  // 列表请求
    TYPE_GET = 3,   // 文件请求
    TYPE_STORE = 4  // 存储请求
};

// 执行计算任务
void cloud_compute(std::string msg);
// 执行连接任务
void cloud_connect(std::string msg);
// 执行存储任务
void cloud_store(std::string msg);
// 执行查看列表任务
void cloud_list(std::string msg);
// 执行获取文件任务
void cloud_getfile(std::string msg);
// 建立连接
bool get_connect();
// run
void cloud_run();
// 接收报文
void recv_msg();
// 处理收到的报文
int analyze_msg(std::string &recvbuf);

#endif