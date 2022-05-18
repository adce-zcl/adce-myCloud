#include "proxy.h"
#include <iostream>
int main(int argc, char **argv)
{
    // 与两台服务器建立连接
    if (!connect_to_servers())
    {
        show_info("Connect to servers false.");
        exit(-1);
    }
    show_info("Connect to servers sucess.");

    // 启动监听
    if (!set_listening())
    {
        show_info("Listen false.");
        exit(-1);
    }
    show_info("Set listening sucess.");

    // 循环处理事件
    show_info("Start epoll loop.");
    if (!epoll_loop())
    {
        show_info("Epoll_loop false.");
        exit(-1);
    }
}