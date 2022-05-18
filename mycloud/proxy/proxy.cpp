#include "proxy.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <fcntl.h>
int serverSocket1;          // 与服务器1相连的套接字
int serverSocket2;          // 与服务器2相连的套接字
int listenSocket;           // 监听用的套接字
std::unordered_set<int> server1Clients;     // 服务端1负载的客户们
std::unordered_set<int> server2Clients;     // 服务端2负载的客户们
// 打印消息
void show_info(std::string info)
{
    std::cout << info << std::endl;
}

// 启动连接
bool connect_to_servers()
{
    serverSocket1 = socket(AF_INET, SOCK_STREAM, 0);
    serverSocket2 = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket1 < 0 || serverSocket2 < 0)
    {
        perror("socket");
        return false;
    }
    // 先与两台服务器建立连接
    struct sockaddr_in serveraddr1, serveraddr2;
    bzero(&serveraddr1, sizeof(serveraddr1));
    bzero(&serveraddr2, sizeof(serveraddr2));
    serveraddr1.sin_family = AF_INET;
    serveraddr2.sin_family = AF_INET;
    serveraddr1.sin_port = htons(SERVER1_PORT);
    serveraddr2.sin_port = htons(SERVER2_PORT);
    inet_pton(AF_INET, SERVER1_IP, &serveraddr1.sin_addr.s_addr);
    inet_pton(AF_INET, SERVER2_IP, &serveraddr2.sin_addr.s_addr);
    if(connect(serverSocket1, (struct sockaddr *)&serveraddr1, sizeof(serveraddr1)) < 0)
    {
        perror("connect to serverSocket1 [124.222.232.238]");
        return false;
    }
    if(connect(serverSocket2, (struct sockaddr *)&serveraddr2, sizeof(serveraddr2)) < 0)
    {
        perror("connect to serverSocket2 [47.99.53.62]");
        return false;
    }
    int ops = fcntl(serverSocket1, F_GETFL);
    ops |= O_NONBLOCK;
    fcntl(serverSocket1, F_SETFL, ops);
    ops = fcntl(serverSocket2, F_GETFL);
    ops |= O_NONBLOCK;
    fcntl(serverSocket2, F_SETFL, ops);
    return true;
}

// 启动监听
bool set_listening()
{
    listenSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (listenSocket < 0)
    {
        perror("socket");
        return false;
    }
    struct sockaddr_in proxyaddr;
    bzero(&proxyaddr, sizeof(proxyaddr));
    proxyaddr.sin_family = AF_INET;
    proxyaddr.sin_port = htons(PROXY_PORT);
    inet_pton(AF_INET, PROXY_IP, &proxyaddr.sin_addr.s_addr);
    
    if (bind(listenSocket, (struct sockaddr*)&proxyaddr, sizeof(proxyaddr)))
    {
        perror("bind");
        return false;
    }

    if (listen(listenSocket, 200) < 0)
    {
        perror("listen");
        return false;
    }
    return true;
}

/**
 * @brief 从服务报文中解析出type,clientsocket,并裁剪报文内容
 * 报文由 type + "\n\r" + "OK" + "\n\t" + "clientsocket" + "\n\r"
 * 变成： type + "\n\r" + "OK" + "\n\t"
 * @param msg 
 * @return int 
 */
int analyze_server_msg(std::string &msg)
{
    int pos = msg.find_last_of("\n\r");
    msg.erase(msg.begin() + pos - 1, msg.end());
    pos = msg.find_last_of("\n\r");
    std::string str_sock(msg.begin() + pos, msg.end());
    int clientSocket = std::stoi(str_sock);
    msg.erase(msg.begin() + pos + 1, msg.end());
    return clientSocket;
}

/**
 * @brief 处理从客户端套接字收到的报文
 * 直接把客户套接字拼接到报文后面，发送给指定的服务器即可
 * @param clientSocket 
 * @return int 
 * 0：客户端开连接
 * -1：出错
 * 1：正常
 */
int manage_client_msg(int clientSocket)
{
    std::string recvbuf;
    char buf[BUFSIZE];
    // 循环接收报文
    while (true)
    {
        int n = read(clientSocket, buf, BUFSIZE);
        if (n > 0)
        {
            std::string tempbuf(buf);
            recvbuf += tempbuf;
        }
        else if (n == 0)    // 客户端断开连接
        {
            return 0;
        }
        else if (n < 0 && errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)    // 出错
        {
            return -1;
        }
        else   // 没有数据
        {
            break;  
        }
    }
    
    recvbuf = recvbuf + std::to_string(clientSocket) + "\n\r";
    int n;
    // 确定是往哪个服务器发送
    if (server1Clients.count(clientSocket) != 0) 
    {
        n = write(serverSocket1, recvbuf.c_str(), recvbuf.length());
    }
    else
    {
        n = write(serverSocket2, recvbuf.c_str(), recvbuf.length());
    }
    if (n < recvbuf.length())
    {
        return -1;
    }
    return 1;
}

/**
 * @brief 处理从服务器发来的消息
 * 要处理报文尾部的客户端套接字信息
 * @param serverSocket 
 * @return int
 * 0：服务器断开连接
 * -1：出错
 * 1：正确
 */
int manage_server_msg(int serverSocket)
{
    std::string recvbuf;
    char buf[BUFSIZE];
    // 循环接收报文
    while (true)
    {
        int n = read(serverSocket, buf, BUFSIZE);
        if (n > 0)
        {
            std::string tempbuf(buf);
            recvbuf += tempbuf;
            bzero(buf, BUFSIZE);
        }
        else if (n == 0)    // 服务器断开连接
        {
            return 0;
        }
        else if (n < 0 && errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)    // 出错
        {
            return -1;
        }
        else   // 没有数据
        {
            break;  
        }
    }
    // 裁剪报文，留下报文的目标客户套接字和主要信息
    int clietnSocket = analyze_server_msg(recvbuf);
    int n = write(clietnSocket, recvbuf.c_str(), recvbuf.length());
    if (n < recvbuf.length())
    {
        return -1;
    }
    return 1;
}

// 循环处理事件
bool epoll_loop()
{
    int epfd = epoll_create(1226);  // 2.6.8内核之后数字没有意义，只是喜欢这个数字而已
    struct epoll_event ev;
    struct epoll_event events[1024];
    ev.data.fd = listenSocket;
    ev.events = EPOLLIN;
    // 这三个套接字都是非阻塞的
    epoll_ctl(epfd, EPOLL_CTL_ADD, listenSocket, &ev);
    ev.data.fd = serverSocket1;
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, serverSocket1, &ev);
    ev.data.fd = serverSocket2;
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, serverSocket2, &ev);
    show_info("Epoll init success, start loop.");
    while (true)
    {
        int nfds = epoll_wait(epfd, events, 1024, -1);
        show_info("Have events occurrence.");
        // 处理所有发生的事件
        for (int i = 0; i < nfds; ++i)
        {
            // 如果是listensocket可读,说明有连接上来
            if ((events[i].data.fd == listenSocket) && (events[i].events & EPOLLIN))  
            {
                // 接受连接
                int clientfd = accept(listenSocket, NULL, NULL);
                if (clientfd < 0)
                {
                    perror("accept");
                    return false;
                }
                // 把clientfd设置为非阻塞
                int opts = fcntl(clientfd, F_GETFL);
                opts |= O_NONBLOCK;
                fcntl(clientfd, F_SETFL, opts);
                // 注册到epoll
                ev.data.fd = clientfd;
                ev.events = EPOLLIN;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev);
                // 给客户端指定服务的服务器
                if (server1Clients.size() <= server2Clients.size())
                {
                    server1Clients.insert(clientfd);
                }
                else
                {
                    server2Clients.insert(clientfd);
                }
                show_info("Have client connect [" + std::to_string(clientfd) + "].");
            }
            // 服务器发来了消息
            else if (events[i].data.fd == serverSocket1 || events[i].data.fd == serverSocket2)  
            {
                int serverSocket = events[i].data.fd;
                int status = manage_server_msg(serverSocket);
                if (status <= 0)
                {
                    show_info("Server disconnect [" + std::to_string(serverSocket) + "].");
                    // 这里可以添加容错 —— 把一个服务器的管理套接字对接到另一台服务器上
                }
                else
                {
                    show_info("Recv server message [" + std::to_string(serverSocket) + "].");
                }
            }
            // 客户端发来了消息
            else if (events[i].events & EPOLLIN)    
            {
                int clientfd = events[i].data.fd;
                if (clientfd < 0)
                {
                    continue;
                }
                // 使用clienfd接收报文并处理报文
                // recv_from_client 可能==-1, ==0, ==1, 因为是非阻塞，要对这三种条件都判断，客户端断开连接也是套接字可读
                int status = manage_client_msg(clientfd);
                if (status <= 0) // 要么是关闭套接字，要么是read出错
                {
                    events[i].data.fd = -1;
                    close(clientfd);
                    // 从服务器取消注册
                    if (server1Clients.count(clientfd) != 0)
                    {
                        server1Clients.erase(clientfd);
                    }
                    else
                    {
                        server2Clients.erase(clientfd);
                    }
                    show_info("Client disconnect [" + std::to_string(clientfd) + "].");
                }
                else
                {
                    show_info("Recv client message [" + std::to_string(clientfd) + "].");
                }
            }
        }
    }
}
