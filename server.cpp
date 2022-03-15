#include "server.h"

server::server(int port, string ip): server_port(port),server_ip(ip) {}

//析构
server::~server() 
{
	for(auto conn:sock_arr)
		close(conn);
	close(server_sockfd);
}

//服务器开始服务
void server::run()
{
	//定义sockfd
	server_sockfd = socket(AF_INET, SOCK_STREAM, 0);

	//定义sockaddr_in
	struct sockaddr_in server_sockaddr;
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_port = htons(server_port);
	server_sockaddr.sin_addr.s_addr = inet_addr(server_ip.c_str());

	//bind 0:s -1:f
	if (bind(server_sockfd, (struct sockaddr *)&server_sockaddr, sizeof(server_sockaddr))==-1)
	{
		perror("bind");
		exit(1);
	}

	//listen 0:s  -1:f
	if(listen(server_sockfd,20) == -1)
	{
		perror("listen");//输出错误原因
		exit(1);//结束程序
	}

	//客户端套接字
	struct sockaddr_in client_addr;
	socklen_t length = sizeof(client_addr);

	//不断取出新连接并创建子线程为其服务
	while(1)
	{
		int conn = accept(server_sockfd, (struct sockaddr*)&client_addr, &length);
		std:: cout<<conn;
		if(conn<0)
		{
			perror("accept");
			exit(1);
		}
		cout<<"文件描述符为："<<conn<<"的客户端成功连接\n";
		sock_arr.push_back(conn);
		//创建线程
		thread t(server::RecvMsg, conn);
		t.detach();		//置为分离状态，不可用join,join会导致主线程阻塞
	}
}

//子线程工作的静态函数
//不可加static(编译器报错)
void server::RecvMsg(int conn)
{
	//接收缓冲区
	char buffer[1024];
	//不断接收数据
	while(1)
	{
		memset(buffer, 0, sizeof(buffer));
		int len = recv(conn, buffer, sizeof(buffer), 0);
		if(strcmp(buffer, "exit") == 0 || len<0)
			break;
		cout<<"收到套接字的描述符为："<<conn<<"发来消息："<<buffer<<endl;
	}
}
