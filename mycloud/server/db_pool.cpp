#include "db_pool.h"
#define DBCONFPATH "./db.conf"

// 静态变量初始化
DB_POOL *DB_POOL::db_pool = nullptr;
unsigned int DB_POOL::count = 0;

// 初始化函数
DB_POOL* DB_POOL::db_pool_init()
{
    if (db_pool == nullptr)
    {
        db_pool = new DB_POOL();
    }
    ++count;
    return db_pool;
}

// 读取配置文件
bool DB_POOL::db_get_info()
{
    std::fstream f;
    f.open(DBCONFPATH);
    if (!f.is_open())
    {
        show_info("Open db.conf false.");
        return false;
    }
    std::string linebuf;
    while (!f.eof())
    {
        std::getline(f, linebuf);
        int pos = linebuf.find_first_of(":");
        std::string key(linebuf.begin(), linebuf.begin() + pos);
        std::string value(linebuf.begin() + pos + 1, linebuf.end());
        // port:3306
        // user:adce
        // passwd:zcl001201
        // hostw:124.222.232.238
        // hostr:47.99.53.62
        // size:128
        if ("port" == key)
        {
            port = std::stoi(value);
        }
        else if ("database" == key)
        {
            db = value;
        }
        else if ("user" == key)
        {
            name = value;
        }
        else if ("passwd" == key)
        {
            passwd = value;
        }
        else if ("hostw" == key)
        {
            host_wirte = value;
        }
        else if ("hostr" == key)
        {
            host_read = value;
        }
        else if ("size" == key)
        {
            maxSize = std::stoi(value);
        }
    }
    return true;
}

// 构造函数
DB_POOL::DB_POOL()
{
    // 读取配置文件
    if(!db_get_info())
    {
        exit(-1);
    }
    // 循环创建链接
    for (int i = 0; i < maxSize; ++i)
    {
        mysqlpp::TCPConnection *conn1 = new mysqlpp::TCPConnection(host_wirte.c_str(), db.c_str(), name.c_str(), passwd.c_str());
        mysqlpp::TCPConnection *conn2 = new mysqlpp::TCPConnection(host_read.c_str(), db.c_str(), name.c_str(), passwd.c_str());
        if (conn1 == nullptr || conn2 == nullptr)
        {
            show_info("TCPConnection false.");
            exit(-1);
        }
        write_pool.push(conn1);
        read_pool.push(conn2);
    }
}

/**
 * @brief Get the connect object
 * 从连接池里获取一个连接，flag代表是需要读池里的连接池，还是写池里连接池
 * @param flag 
 * 1:read
 * 2:write
 * @return mysqlpp::Connection* 
 */
mysqlpp::TCPConnection* DB_POOL::get_connect(int flag)
{
    mysqlpp::TCPConnection *conn;
    if (1 == flag)
    {
        std::unique_lock<std::mutex> lk(mut);
        cond.wait(lk, [this]() { return !read_pool.empty(); });
        conn = read_pool.front();
        read_pool.pop();
        lk.unlock();
    }
    else
    {
        std::unique_lock<std::mutex> lk(mut);
        cond.wait(lk, [this]() { return !write_pool.empty(); });
        conn = write_pool.front();
        write_pool.pop();
        lk.unlock();
    }
    return conn;
}

/**
 * @brief 把链接归还到指定的池子里
 * @param connect 
 * @param flag 
 * 1:read
 * 2:write
 */
void DB_POOL::put_connect(mysqlpp::TCPConnection *connect, int flag)
{
    if (1 == flag)
    {
        std::unique_lock<std::mutex> lk(mut);
        read_pool.push(connect);
        cond.notify_one();
        lk.unlock();
    }
    else
    {
        std::unique_lock<std::mutex> lk(mut);
        write_pool.push(connect);
        cond.notify_one();
        lk.unlock();
    }
    return;
}

// 析构函数
DB_POOL::~DB_POOL()
{
    for (int i = 0; i < maxSize; ++i)
    {
        mysqlpp::TCPConnection *conn1 = write_pool.front();
        conn1->disconnect();
        mysqlpp::TCPConnection *conn2 = read_pool.front();
        conn2->disconnect();
        delete conn1;
        delete conn2;
        write_pool.pop();
        read_pool.pop();
    }
}

// 打印消息
void show_info(std::string info)
{
    std::cout << info << std::endl;
}