#include "cloud.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>

// 线程同步用的互斥锁和条件变量
std::mutex socketmut;
std::condition_variable socketcond;
bool canRead;

// 与代理服务器通信用的套接字
int serverSocket;
std::string recvbuf;

// 数据库连接池指针
DB_POOL *db_pool = nullptr;

// 连接套接字
bool get_connect()
{
    int listenSocket;
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0)
    {
        perror("socket");
        return false;
    }

    struct sockaddr_in serveraddr;
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(PORT);
    serveraddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
    {
        perror("bind");
        return false;
    }

    if (listen(listenSocket, 20) < 0)
    {
        perror("listen");
        return false;
    }

    serverSocket = accept(listenSocket, NULL, NULL);
    if (serverSocket < 0)
    {
        perror("accept");
        return false;
    }
    int ops = fcntl(serverSocket, F_GETFL);
    ops |= O_NONBLOCK;
    fcntl(serverSocket, F_SETFL, ops);
    close(listenSocket);
    return true;
}

// 逻辑业务函数——主函数
void cloud_run()
{
    db_pool = DB_POOL::db_pool_init();
    if (db_pool == nullptr)
    {
        show_info("Database pool init false.");
        exit(-1);
    }
    show_info("Database init success.");

    // recvThread线程——负责接收报文
    std::thread recvThread(recv_msg);

    // 线程池——负责根据报文处理任务
    thread_pool pool;
    while (true)
    {
        std::unique_lock<std::mutex> socketlock(socketmut);
        socketcond.wait(socketlock, [&](){ return canRead; });  // 检测是否可读
        std::string msg = recvbuf;
        // 解析数据包，从数据包中解析出type
        int type = analyze_msg(msg);
        // 根据类型不同，submit不同的任务,这里可以应用哪种设计模式？工厂模式还是适配器？
        switch (type)
        {
        case TYPE_CON:
        {
            pool.submit(cloud_connect, msg);
            break;
        }  
        case TYPE_COM:
        {
            pool.submit(cloud_compute, msg);
            break;
        }
        case TYPE_SHOW:
        {
            pool.submit(cloud_list, msg);
            break;
        }
        case TYPE_GET:
        {
            pool.submit(cloud_getfile, msg);
            break;
        }
        case TYPE_STORE:
        {
            pool.submit(cloud_store, msg);
            break;
        }
        default:
            break;
        }
        canRead = false;
        socketlock.unlock();
    }
    recvThread.join(); // 这里永远执行不到
}

// 使用select，当serverSocket可读时，对serverSocket加锁读
// 不能阻塞使用serverSocket，因为当serverSocket被read阻塞的时候，会影响它的写功能
void recv_msg()
{
    fd_set serverSocketSet;
    FD_ZERO(&serverSocketSet);
    while (true)
    {
        FD_SET(serverSocket, &serverSocketSet);
        // 只监听serverSocket的可读事件
        select(serverSocket + 1, &serverSocketSet, NULL, NULL, NULL);
        show_info("recvThread: have recv msg.");
        // serverSocket的读事件触发
        char buf[BUFSIZE];
        bzero(buf, BUFSIZE);
        int n;
        std::unique_lock<std::mutex> socketlock(socketmut);   // 上锁
        recvbuf.clear();    // 清空buf

        while (true)
        {
            n = read(serverSocket, buf, BUFSIZE);
            if (n > 0)
            {
                std::string temp(buf);
                recvbuf += temp;
                bzero(buf, BUFSIZE);
            }
            else if (n < 0 && errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)  // 出错
            {
                show_info("recvThread: read false.");
                exit(-1);
            }
            else if (n == 0)    // 断开连接
            {
                show_info("recvThread: proxy had disconnect.");
                exit(-1);
            }
            else    // 没有数据可读
            {
                break;
            }
        }
        // 提交recvbuf
        canRead = true;
        socketcond.notify_one();
        socketlock.unlock();
    }
}

/**
 * @brief 从报文中解析出type
 * 报文由 type + "\n\r" + 实体 + "\n\r" + clientsocket + "\n\r"
 * 变成： 实体 + "\n\r" + clientsocket + "\n\r"
 * @param msg 
 * @return int 
 */
int analyze_msg(std::string &msg)
{
    int type;
    int pos = msg.find_first_of("\n\r");
    std::string str_type(msg.begin(), msg.begin() + pos);
    type = std::stoi(str_type);
    msg.erase(msg.begin(), msg.begin() + pos + 2);
    return type;
}

/**
 * @brief 执行连接任务
 * 查看数据库中账户是否已经存在
 * 操作主数据库 —— 写
 * @param msg 
 */
void cloud_connect(std::string msg)
{
    // 1.解析出name和password
    // 2.连接数据库验证
    // 3.如果存在，验证密码是否正确，如果不存在，新建账户
    // 4.将数据库主键ID返回
    // 5.拼接报文，发送回去

    // 解析报文,从报文中解析出 name password clientsocket
    int pos = msg.find_first_of("\n\r");
    std::string _name(msg.begin(), msg.begin() + pos);
    msg.erase(msg.begin(), msg.begin() + pos);
    pos = msg.find_first_of("\n\r");
    std::string _password(msg.begin(), msg.begin() + pos);
    msg.erase(msg.begin(), msg.begin() + pos);
    pos = msg.find_first_of("\n\r");
    std::string clientsocket(msg.begin(), msg.begin() + pos);

    // 从数据库连接池中取出连接
    mysqlpp::TCPConnection *conn = db_pool->get_connect(1);

    // 要发送的报文
    std::string sendbuf;

    // 查询这个数据是否在数据库里
    std::string sql = "select id, name, passwd from tb_user where name='" + _name + "';";
    mysqlpp::Query query = conn->query();
    mysqlpp::StoreQueryResult res = query.store(sql, sql.length());
    int num = res.num_rows();
    if (num == 0)   // 不存在
    {
        // 创建用户
        sql = "inster into tb_user (name, passwd) values('" + _name + "','" + _password + "');";
        query << sql;
        query.store();
        sql = "select id from tb_user where name='" + _name + "';";
        res = query.store(sql, sql.length());
        int id = res.at(0)["id"];
        // 发送报文
        sendbuf = std::to_string(id) + "\n\rNEW\n\r" + clientsocket + "\n\r";
    }
    else                            // 确实存在此账户
    {
        // 验证密码是否正确
        std::string password(res.at(0)["passwd"]);
        if (password == _password)  // 密码正确
        {
            sendbuf = std::string(res.at(0)["id"]) + "\n\rOK\n\r" + clientsocket + "\n\r";
        }
        else                        // 密码不正确
        {
            sendbuf = "-1\n\rPASS\n\r" + clientsocket + "\n\r";
        }
    }
    // 发送
    int n = write(serverSocket, sendbuf.c_str(), sendbuf.length());
    if (n < sendbuf.length())
    {
        perror("write");
        return;
    }
    return;
}

/**
 * @brief 执行存储任务
 * @param msg 
 */
void cloud_store(std::string msg)
{

}

/**
 * @brief 执行查看列表任务
 * 
 * @param msg 
 */
void cloud_list(std::string msg)
{

}

/**
 * @brief 执行获取文件任务
 * 
 * @param msg 
 */
void cloud_getfile(std::string msg)
{

}

/**
 * @brief 执行计算任务
 * 
 * @param msg 
 */
void cloud_compute(std::string msg)
{

}