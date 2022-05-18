# README

1. 因为线程池和线程安全队列是模板，鉴于C++模板的特殊编译方式，将.cpp文件和.h文件合并为.hpp文件这也是标准模板编译的标准方式之一。
2. libstdc++库为6.0.25,可能还需要安装一些比较新版本的库.
3. 数据库连接使用MySQL++第三方库.
4. 数据库连接池加载的时候会从db.conf文件中读取链接必要的信息，因为是在数据库连接池的构造函数中读取配置文件，按照编程规范，构造函数不应该报错，所以应确保db.conf文件的正确性.
5. 连接功能理论上已经完毕,但是没有编译调试,因为要做go的项目,而go web环境对我来说是陌生的,到六月中旬之前我可能每天都要在go了,这个项目就暂停到六月中旬之后.
6. 根据自己的环境可能需要重写makefile

# 有想法但是待完善的功能

1. 客户端用Qt封装成可视化界面.
2. 代理服务器使用Redis做一次缓存,使用redisplusplus或hiredis第三方库.
3. 代理服务器和服务器应作为守护进程.
4. 应把show_info()函数修改为日志输出.

# 知识点和将要涉及的知识点

## 客户端

1. socket通信
2. 单例模式
3. Qt框架(待完成)

## 代理服务器

1. epoll非阻塞模型
2. socket通信
3. Redis(待完成)

## 服务器

1. select非阻塞模型
2. C++11线程池
3. 线程安全队列模板
4. MySQL数据库连接池
5. 单例模式
6. 文件目录解析(待开发功能模块的知识点)