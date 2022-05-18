#include "adce_cloud.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>

// 静态成员初始化
int ADCE_CLOUD::count = 0;
ADCE_CLOUD* ADCE_CLOUD::adce_cloud = nullptr;

/**
 * @brief 初始化函数
 * 
 * @return ADCE_CLOUD* 
 */
ADCE_CLOUD *ADCE_CLOUD::adce_cloud_init()
{
    if (adce_cloud == nullptr)
    {
        adce_cloud = new ADCE_CLOUD();
    }
    count++;
    return adce_cloud;
}

// 判断是否已经连接
bool ADCE_CLOUD::get_status()
{
    return isConnect;
}

// 获取名字
std::string ADCE_CLOUD::get_name()
{
    return name;
}

// 析构函数
ADCE_CLOUD::~ADCE_CLOUD()
{
    // 释放连接
    ADCE_CLOUD::count--;
    if (ADCE_CLOUD::count == 0)
    {
        close(proxySocket);
        delete ADCE_CLOUD::adce_cloud;
    }
}

/**
 * @brief Get the Connect object
 * 
 * @return int 
 * >0   :success
 * <=0  :false
 */
int getConnect()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
        return -1;
    }
    struct sockaddr_in serveraddr;
    bzero(&serveraddr, 0);
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(PROXYPORT);
    inet_pton(AF_INET, PROXYIP, &serveraddr.sin_addr.s_addr);
    if (connect(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
    {
        perror("connect");
        return -1;
    }
    int ops = fcntl(sockfd, F_GETFL);
    ops |= O_NONBLOCK;
    fcntl(sockfd, F_SETFL, ops);
    return sockfd;
}


/**
 * @brief 发送报文,并把收到的报文保存在recvbuf中
 * 
 * @param sendbuf 
 * @param recvbuf 
 * @return void
 * 如果出错或未按照预期收到报文，则把recvbuf清空
 */
void ADCE_CLOUD::cloud_sendrecv_msg(std::string &sendbuf, std::string &recvbuf)
{
    // 发送
    size_t sendBufLen = sendbuf.length();
    if (write(proxySocket, sendbuf.c_str(), sendBufLen) < sendBufLen)
    {
        perror("write");
        return;
    }
    std::string buf;
    char tempRecvBuf[1024];
    bzero(tempRecvBuf, 1024);
    int recvSize;
    int count = 5;      // 计时，不想无限等待
    while (recvSize = read(proxySocket, tempRecvBuf, 1024))     // proxySocket为非阻塞
    {
        if (recvSize > 0)
        {
            std::string curmsg(tempRecvBuf);
            buf += curmsg;
            bzero(tempRecvBuf, 1024);
        }
        else if (recvSize < 0 && errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)    // read出错
        {
            perror("read");
            recvbuf.clear();
            return;
        }
        else        // 没有数据在读了
        {
            break;
        }
        sleep(1);   // 阻塞一秒钟
        --count;
        if (count == 0)
        {
            recvbuf.clear();
            return;
        }
    }
    // 返回
    recvbuf = buf;
    return;
}

/**
 * @brief 连接代理服务器
 * @param _name 
 * @param _password 
 * @return int 
 * 1：连接成功
 * 0：密码错误
 * 2：新建账户
 * -1：连接错误
 */
int ADCE_CLOUD::cloud_connect(std::string &_name, std::string &_password)
{
    // 如果已经建立连接，则不再重复建立
    if (isConnect == true)
    {
        return 1;
    }
    // 建立TCP连接
    proxySocket = getConnect();
    if (proxySocket < 0)
    {
        return -1;
    }

    // 发送一个"建立连接"的报文
    std::string sendbuf = std::to_string(TYPE_CON) + "\n\r" + _name + "\n\r" + _password + "\n\r";
    std::string recvbuf;

    // 发送报文,并把收到的报文保存在recvbuf中
    cloud_sendrecv_msg(sendbuf, recvbuf);
    if (recvbuf.size() == 0)    // 没有如期收到回复的数据包
    {
        return -1;
    }

    // 从recvbuf中解析出id
    int pos = recvbuf.find_first_of("\n\r");
    std::string str_id(recvbuf.begin(), recvbuf.begin() + pos);
    int _id = std::stoi(str_id);

    // 处理返回结果
    if (_id > 0)
    {
        id = _id;
        isConnect = true;
        name = _name;
        password = _password;
        // 处理接受的报文
        if (recvbuf.find("OK") != std::string::npos)
        {
            return 1;
        }
        else if (recvbuf.find("NEW") != std::string::npos)
        {
            isConnect = true;
            return 2;
        }
    }
    else
    {
        close(proxySocket);
        return 0;
    }
    return 1;
}
