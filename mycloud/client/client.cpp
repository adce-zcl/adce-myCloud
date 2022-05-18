/**
 * @file client.cpp
 * @author adce (adce@qq.com)
 * @brief 客户端代码
 * @version 0.1
 * @date 2022-05-09
 * 
 */
#include "adce_cloud.h"
#include <string>
#include <iostream>
#include <unistd.h>
// 显示列表
void show_help();
// 连接任务
void client_connect();
// 计算任务
void client_comput();
// 查看列别
void client_show();
// 存储文件
void clinet_store();
// 读取文件
void client_getfile();

ADCE_CLOUD *adce_cloud = ADCE_CLOUD::adce_cloud_init();
int main(int argc, char **argv)
{
    std::cout << "\n\n\t<< Welcome ADCE CLOUD >>" << std::endl;
    show_help();
    while (true)
    {
        int key;
        std::string flag = adce_cloud->get_name();
        if (flag == "")
        {
            flag = "^_^";
        }
        std::cout << "[ " << flag << " @ADCE CLOUD]$ ";
        std::cin >> key;
        switch (key)
        {
        case 0:
        {
            client_connect();
            break;
        }
        case 1:
        {
            client_comput();
            break;
        }
        case 2:
        {
            client_show();
            break;
        }
        case 3:
        {
            clinet_store();
            break;
        }
        case 4:
        {
            client_getfile();
            break;
        }
        case 5:
        {
            show_help();
            break;
        }
        default:
        {
            std::cout << "Invalid input!" << std::endl;
            break;
        }  
        }
    }
    return 0;
}

// 连接任务
void client_connect()
{
    std::string name;
    char *pass;
    std::cout << "Input your name:";
    std::cin >> name;
    pass = getpass("Input your password:");
    std::string password(pass);
    delete pass;
    int status = adce_cloud->cloud_connect(name, password);
    if (1 == status)
    {
        std::cout << "Cloud connect success, welcome [" << name << "]." << std::endl;
    }
    else if (2 == status)
    {
        std::cout << "Have create new account (name:" << name << ", password: ****)." << std::endl;
    }
    else if (0 == status)
    {
        std::cout << "Cloud connect failed, password error." << std::endl;
    }
    else
    {
        std::cout << "Cloud connect failed, unknow error." << std::endl;
    }
    return;
}

// 计算任务
void client_comput()
{
    if (!adce_cloud->get_status())  // 如果没有连接
    {
        std::cout << "Can not connect, please connect first." << std::endl;
        return;
    }
    std::cout << "The function is not realized." << std::endl;
}

// 查看列别
void client_show()
{
    if (!adce_cloud->get_status())  // 如果没有连接
    {
        std::cout << "Can not connect, please connect first." << std::endl;
        return;
    }
    std::cout << "The function is not realized." << std::endl;
}

// 存储文件
void clinet_store()
{
    if (!adce_cloud->get_status())  // 如果没有连接
    {
        std::cout << "Can not connect, please connect first." << std::endl;
        return;
    }
    std::cout << "The function is not realized." << std::endl;
}

// 读取文件
void client_getfile()
{
    if (!adce_cloud->get_status())  // 如果没有连接
    {
        std::cout << "Can not connect, please connect first." << std::endl;
        return;
    }
    std::cout << "The function is not realized." << std::endl;
}

// 显示列表
void show_help()
{
    std::cout << "---------------------------------" << std::endl;
    std::cout << "| please input key for function:|" << std::endl;
    std::cout << "| [0]:connect.\t\t\t|\n| [1]:computer.\t\t\t|\n| [2]:show contents.\t\t|\n| [3]:save file.\t\t|\n| [4]:watch file.\t\t|\n| [5]:show help.\t\t|\n";
    if (adce_cloud->get_status())
    {
        std::cout << "| status[conncet]\t\t|\n";
    }
    else
    {
        std::cout << "| status[false]\t\t\t|\n";
    }
    std::cout << "---------------------------------" << std::endl;
}