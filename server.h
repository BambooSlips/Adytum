#ifndef SERVER_H
#define SERVER_H

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

#include "global.h"

using namespace std;

class server {
	private:
		int server_port;			//服务器端口
		int server_sockfd;			//置为listen状态的套接字描述符
		string server_ip;			//服务器ip
		static vector<bool> sock_arr;		//保存所有套接字描述符
		static unordered_map<string, int> name_sock_map;	//名字和套接字描述符
		static pthread_mutex_t name_sock_mutx;			//互斥锁 name_sock_map
		static unordered_map<int, set<int> > group_map;		//记录群号和套接字描述符集合
		static unordered_map<string, string> from_to_map;       //记录一用户向另一用户发送的信息
		static pthread_mutex_t group_mutx;			//互斥锁 group_map
		static pthread_mutex_t from_mutex;			//互斥锁 from_to_map
	public:
		server(int port, string ip);		
		~server();
		void run();				//服务器工作
		static void RecvMsg(int epollfd, int conn);		//子线程工作的静态函数
		static void HandleRequest(int epollfd, int conn, string str, tuple<bool, string, string, int, int> &info);
		static void setnonblocking(int conn);	//将套接字设为非阻塞
};

#endif
