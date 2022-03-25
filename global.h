#ifndef _GLOBAL_H
#define _GLOBAL_H

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <iostream>
#include <thread>
#include <vector>
#include <mysql/mysql.h>
#include <unordered_map>
#include <pthread.h>
#include <set>				//将处于同一群聊的套接字描述符放入同一个set之中
#include <hiredis/hiredis.h>		//连接redis
#include <fstream>			//读取本地文件
#include <sys/epoll.h>			//使用epoll、boost实现reactor模式的并发服务器
#include <boost/bind/bind.hpp>		//using <boost/bind/bind.hpp> instead of <boost/bind.hpp>
#include <boost/asio.hpp>
#include <errno.h>			

using namespace std;

#endif
