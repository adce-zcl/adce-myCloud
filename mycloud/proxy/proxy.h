// 新增功能：1.记录连接过的所有客户 id - server 映射    // 这里可以用redis
// 增加name - password 的缓存？

#ifndef PROXY_H_
#define PROXY_H_
#include <vector>
#include <iostream>
#include <string>
#include <unordered_set>

#define PROXY_IP "124.222.232.238"
#define PROXY_PORT 1227
#define SERVER1_IP "124.222.232.238"
#define SERVER1_PORT 1226
#define SERVER2_IP "47.99.53.62"
#define SERVER2_PORT 1226
#define BUFSIZE (1500 - 20 - 20)
// 报文的类型
enum
{
    TYPE_CON = 0,   // 连接请求
    TYPE_COM = 1,   // 计算请求
    TYPE_SHOW = 2,  // 列表请求
    TYPE_GET = 3,   // 文件请求
    TYPE_STORE = 4  // 存储请求
};

// 打印消息
void show_info(std::string info);
// 与服务器建立连接
bool connect_to_servers();
// 启动监听
bool set_listening();
// 等待客户连接
bool epoll_loop();
// 处理从客户端套接字收到的报文
int manage_client_msg(int clientSocket);
// 从服务器发来的报文中解析出客户端套接字
int analyze_server_msg(std::string &msg);
// 处理从服务器发来的消息
int manage_server_msg(int serverSocket);

#endif