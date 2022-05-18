/**
 * @file adce_cloud.h
 * @author adce (adce@qq.com)
 * @brief 
 * 1.给用户暴露功能接口
 * 2.单例模式
 * @version 0.1
 * @date 2022-05-09
 */
#ifndef CLIENT_H_
#define CLIENT_H_
#include <vector>
#include <string>
#define PROXYIP "124.222.232.238"   // 代理服务器的IP地址
#define PROXYPORT 1227              // 代理服务器的端口号
#define BUF_MAX (1500 - 20 - 20 - sizeof(int) * 2)  // 最大的buf长度
// 报文的类型
enum
{
    TYPE_CON = 0,   // 连接请求
    TYPE_COM = 1,   // 计算请求
    TYPE_SHOW = 2,  // 列表请求
    TYPE_GET = 3,   // 文件请求
    TYPE_STORE = 4  // 存储请求
};

// ADCE云
class ADCE_CLOUD
{
    // 删除的函数
    ADCE_CLOUD(const ADCE_CLOUD &other) = delete;
    ADCE_CLOUD(ADCE_CLOUD &&other) = delete;
    ADCE_CLOUD &operator=(const ADCE_CLOUD &other) = delete;
    ADCE_CLOUD &operator=(ADCE_CLOUD &&other) = delete;

private:
    int proxySocket;                // 与代理服务器通信用的套接字,非阻塞
    unsigned int id;                // 随机生成呢？随机生成会重复，必须要从数据库索引返回
    std::string name;               // 用户名
    std::string password;           // 密码
    bool isConnect;                 // 是否连接

    static ADCE_CLOUD *adce_cloud;  // 默认构造函数应该私有，单例模式
    static int count;               // 引用计数

    // 构造函数
    ADCE_CLOUD() : isConnect(false),name(""),password(""),proxySocket(0),id(-1) {}

    // 发送报文,如果有返回结果，把结果保存在recvbuf中，并返回状态码
    void cloud_sendrecv_msg(std::string &sendbuf, std::string &recvbuf);

public:
    // 初始化
    static ADCE_CLOUD *adce_cloud_init();

    // 判断是否已经连接
    bool get_status();

    // 获取名字
    std::string get_name();

    // 连接代理软件
    int cloud_connect(std::string &_name, std::string &_password);

    // 查看当前用户的所有已经保存的文本列表，可以是从数据库
    void cloud_watch_file();

    // 计算，不走数据库
    void cloud_compute_sum(long start, long end);

    // 保存一个新文件到云
    void cloud_write(std::string filePath);

    // 从云端读取一个指定文件内容
    void cloud_read(std::string fileName);

    // 析构函数
    ~ADCE_CLOUD();
};




#endif