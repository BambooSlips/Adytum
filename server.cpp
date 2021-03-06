#include "server.h"

vector<bool> server::sock_arr(10240, false);	//use command "ulimit -n" to check
unordered_map<string, int> server::name_sock_map;
unordered_map<int, set<int> >server::group_map;	//记录群号和套接字描述符集合
unordered_map<string, string> server::from_to_map;	//私聊
pthread_mutex_t server::group_mutx;		//互斥锁
pthread_mutex_t server::name_sock_mutx;
pthread_mutex_t server::from_mutex;		//自旋锁 from_to_map

server::server(int port, string ip): server_port(port),server_ip(ip) 
{
	pthread_mutex_init(&name_sock_mutx, NULL);  //创建互斥锁
	pthread_mutex_init(&group_mutx, NULL);
	pthread_mutex_init(&from_mutex, NULL);
}

//析构
server::~server() 
{
	/*
	   for(auto conn:sock_arr)
	   close(conn);
	   close(server_sockfd);
	   */
	//close the sock that is still open
	for(int i = 0; i < sock_arr.size(); i++)
	{
		if(sock_arr[i])
			close(i);
	}
	close(server_sockfd);
}

//服务器开始服务
void server::run()
{
	//listen的backlog大小
	int LISTENQ = 200;
	int i, maxi, listenfd, connfd, sockfd, epfd, nfds;
	ssize_t n;
	socklen_t client;
	//声明epoll_event结构体变量，eev用于注册事件，数组用于回传要处理的事件
	struct epoll_event ev, events[10240];
	//生成用于处理accept的epoll专用文件描述符
	epfd = epoll_create(10240);
	struct sockaddr_in clientaddr;
	struct sockaddr_in serveraddr;
	listenfd = socket(PF_INET, SOCK_STREAM, 0);
	//把socket设为非阻塞方式
	setnonblocking(listenfd);
	//设置与要处理的事件相关的文件描述符
	ev.data.fd = listenfd;
	//设置要处理的事件类型
	ev.events = EPOLLIN|EPOLLET;
	//注册epoll事件
	epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
	//设置serveraddr
	bzero(&serveraddr,sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(server_ip.c_str());		//服务器ip
	serveraddr.sin_port = htons(server_port);
	bind(listenfd, (sockaddr *)&serveraddr, sizeof(serveraddr));
	listen(listenfd, LISTENQ);
	client = sizeof(clientaddr);
	maxi = 0;

	/* 定义一个10线程的线程池 */
	boost::asio::thread_pool tp(10);

	while (1)
	{
		cout<<"--------------------"<<endl;
		cout<<"epoll_wait阻塞中"<<endl;
		//等待epoll事件发生
		nfds = epoll_wait(epfd, events, 10240, -1);	//-1:timeout 一直阻塞到有事件发生 0:立即返回 x:等待x ms
		cout<<"epoll_wait返回， 有事件发生"<<endl;
		//处理发生的所有事件
		for (i = 0; i < nfds; ++i)
		{
			//有新客户端连接服务器
			if (events[i].data.fd == listenfd)
			{
				connfd = accept(listenfd, (sockaddr *)&clientaddr, &client);
				if (connfd < 0)
				{
					perror("connfd < 0");
					exit(1);
				}
				else
				{
					cout<<"用户"<<inet_ntoa(clientaddr.sin_addr)<<"正在连接\n";
				}
				//设置用于读操作的文件描述符
				ev.data.fd = connfd;
				//设置用于注册的读操作事件，ET边缘触发
				//使用EPOLLONESHOT防止多线程处理同一socket
				ev.events = EPOLLIN|EPOLLET|EPOLLONESHOT;
				//边缘触发将套接字设为非阻塞
				setnonblocking(connfd);
				//注册ev
				epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
			}
			else if (events[i].events&EPOLLIN)
			{
				sockfd = events[i].data.fd;
				events[i].data.fd = -1;
				cout<<"接收到读事件"<<endl;

				string recv_str;
				//加入任务队列，处理事件
				boost::asio::post(boost::bind(RecvMsg, epfd, sockfd));
			}

		}
	}
	close(listenfd);
	/*
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
	*/
}

//子线程工作的静态函数
//不可加static(编译器报错)
void server::RecvMsg(int epollfd, int conn)
{
        //if_login login_name target_name target_conn group_num
	tuple<bool, string, string, int, int> info;
	get<0>(info) = false;	//将if_login置false
	get<3>(info) = -1;	//target_conn置-1

	string recv_str;
	while(1)
	{
		char buf[10];
		memset(buf, 0, sizeof(buf));
		int ret  = recv(conn, buf, sizeof(buf), 0);
		if(ret < 0)
		{
			cout<<"recv返回值小于0"<<endl;
			//对于非阻塞IO，下面的事件成立标识数据已经全部读取完毕
			if((errno == EAGAIN) || (errno == EWOULDBLOCK)){
				printf("数据读取完毕\n");
				cout<<"接收到的完整内容为："<<recv_str<<endl;
				cout<<"开始处理事件"<<endl;
				break;
			}
			cout<<"errno:"<<errno<<endl;
			return;
		}
		else if(ret == 0){
			cout<<"recv返回值为0"<<endl;
			return;
		}
		else{
			printf("接收到内容如下: %s\n",buf);
			string tmp(buf);
			recv_str+=tmp;
		}
	}
	string str=recv_str;
	HandleRequest(epollfd,conn,str,info);

	/*
	   char buffer[1024];
	   while(1)
	   {
	   memset(buffer, 0, sizeof(buffer));
	   int len = recv(conn, buffer, sizeof(buffer), 0);
	   if(strcmp(buffer, "context:exit") == 0 || len <= 0)
	   {
	   close(conn);
	   sock_arr[conn] = false;
	   break;
	   }
	   cout<<"收到套接字描述符为"<<conn<<"发来的消息："<<buffer<<endl;
	   string str(buffer);
	   HandleRequest(conn, str, info);
	   }
	   */
}

void server::HandleRequest(int epollfd, int conn, string str, tuple<bool, string, string, int, int> &info)
{
	char buffer[1024];
	string name, pass;
	bool if_login = get<0>(info);			//记录当前服务对象是否成功登录
	string login_name = get<1>(info);		//记录当前服务对象的名字
	string target_name = get<2>(info);		//记录当前目标对象的名字
	int target_conn = get<3>(info);			//目标对象的套接字描述符
	int group_num = get<4>(info);			//记录所处群号

	//连接MySQL数据库
	MYSQL *con = mysql_init(NULL);

	if (!mysql_real_connect(con, "127.0.0.1", "leaf", "123456789", "adytum", 3306, NULL, CLIENT_MULTI_STATEMENTS))
	{
		cout<<mysql_error(con);
	}

	//连接redis数据库
	redisContext *redis_target = redisConnect("127.0.0.1", 6379);
	if (redis_target -> err)
	{
		redisFree(redis_target);
		cout<<"连接redis失败"<<endl;
	}

	//接收cookie判断redis是否保存该用户的登录状态
	if (str.find("cookie:") != str.npos)
	{
		string cookie = str.substr(7);
		//hget cookie name
		string redis_str = "hget " + cookie + " name";
		redisReply *r = (redisReply*) redisCommand(redis_target, redis_str.c_str());
		string send_res;

		if (r -> str)
		{
			cout<<"查询redis结果:"<<r -> str <<endl;
			send_res = r -> str;
		}
		else
			send_res = "NULL";
		send(conn, send_res.c_str(), send_res.length() + 1, 0);
	}


	//注册
	if(str.find("name:") != str.npos)
	{
		int p1 = str.find("name:"), p2=str.find("pass:");
		name = str.substr(p1+5, p2-5);
		pass = str.substr(p2+5, str.length()-p2-4);
		string search = "INSERT INTO user VALUES (\"";
		search += name;
		search += "\",\"";
		search += pass;
		search += "\");";
		cout<<"sql:"<<search<<endl<<endl;
		//若执行不成功输出错误
		if (!mysql_query(con, search.c_str()))
		{
			cout<<mysql_error(con);
		}
	}
	//登录
	else if(str.find("login")!= str.npos)
	{
		int p1 = str.find("login"), p2 = str.find("pass");
		name = str.substr(p1+5, p2-5);
		pass = str.substr(p2+5, str.length()-p2-4);
		string search = "SELECT * FROM user WHERE NAME=\"";
		search += name;
		search += "\";";
		cout<<"sql:"<<search<<endl;
		auto search_res = mysql_query(con, search.c_str());
		auto result = mysql_store_result(con);
		//get numbers of cols and rows
		int col = mysql_num_fields(result);
		int row = mysql_num_rows(result);
		//if username exists
		if(search_res == 0 && row!= 0)
		{
			cout<<"查询到用户名！\n";
			auto info = mysql_fetch_row(result);	//get the info of a row
			cout<<"查询到密码为："<<info[1]<<"的用户\""<<info[0]<<"\""<<endl;
			//密码正确
			if(info[1] == pass)
			{
				cout<<"The passward is correct!\n";
				string str1 = "ok";
				if_login = true;
				login_name = name;		//record the name of the user logined
				pthread_mutex_lock(&name_sock_mutx);		//上锁
				name_sock_map[login_name] = conn;
				pthread_mutex_unlock(&name_sock_mutx);

				//随机生成sessionid并发送到客户端
				//sessionid 长度为10,由0-9或大小写字母随机组成，
				//理论上(10+26+26)^10种组合
				srand(time(NULL));
				for (int i = 0; i < 10; i++)
				{
					int type = rand()%3;
					if (type == 0)				//数字
						str1 += '0' + rand() % 9;
					else if (type == 1)			//小写字母
						str1 += 'a' + rand() % 26;
					else if (type == 2)			//大写字母
						str1 += 'A' + rand() % 26;

				}

				//将sessionid存入redis
				string redis_str = "hset " + str1.substr(2) + " name " + login_name;
				redisReply *r = (redisReply * ) redisCommand(redis_target, redis_str.c_str());
				//设置生存时间，默认300s
				redis_str = "expire " + str1.substr(2) + " 300";
				r = (redisReply *)redisCommand(redis_target, redis_str.c_str());
				cout<<"随机生成的sessionid为："<<str1.substr(2)<<endl;

				send(conn, str1.c_str(), str1.length()+1, 0);
			}
			//密码错误
			else
			{
				cout<<"The password is incorrect!\n";
				char str1[6] = "wrong";
				send(conn, str1, strlen(str1), 0);
			}
		}
		//the username does not exist
		else
		{
			cout<<"查询失败！\n\n";
			char str1[6] = "wrong";
			send(conn, str1, strlen(str1), 0);
		}
	}
	//设定目标文件描述符
	else if(str.find("target:") != str.npos)
	{
		int pos1 = str.find("from");
		string target = str.substr(7, pos1-7), from = str.substr(pos1+5);
		target_name = target;
		if (name_sock_map.find(target) == name_sock_map.end())
			cout<<"源用户"<<login_name<<"，目标用户"<<target_name<<"尚未登录，无法发起私聊\n";
		else
		{
			pthread_mutex_lock(&from_mutex);
			from_to_map[from] = target;
			cout<<"from"<<from<<" target:"<<target<<endl;
			pthread_mutex_unlock(&from_mutex);
			login_name = from;
			cout<<"源用户"<<login_name<<"向目标用户"<<target_name<<"发起的私聊将建立";
			cout<<"，目标用户的套接字描述符为"<<name_sock_map[target]<<endl;
			target_conn = name_sock_map[target];
		}
	}
	//接收消息并转发
	else if(str.find("content:") != str.npos)
	{
		target_conn = -1;
		cout<<"转发方法：\n";
		//根据两个map找出当前用户和目标用户
		for (auto i:name_sock_map)
		{
			if (i.second == conn)
			{
				login_name = i.first;
				target_name = from_to_map[i.first];
				target_conn = name_sock_map[target_name];
				break;
			}
		}
		if (target_conn == -1)
		{
			cout<<"找不到目标用户"<<target_name<<"的套接字，将尝试重新寻找目标用户的套接字\n";
			if(name_sock_map.find(target_name) != name_sock_map.end())
			{
				target_name = name_sock_map[target_name];
				cout<<"重新查找目标用户套接字成功！\n";
			}
			else
			{
				cout<<"查找仍失败，转发失败！\n";
			}
		}

		string recv_str(str);
		string send_str = recv_str.substr(8);
		cout<<"用户"<<login_name<<"向"<<target_name<<"发送："<<send_str<<endl;
		send_str = "[" + login_name + "]" + send_str;
		send(target_conn, send_str.c_str(), send_str.length(), 0);
	}
	else if (str.find("group:") != str.npos)
	{
		cout<<"绑定群聊号方法\n";
		string recv_str(str);
		string num_str = recv_str.substr(6);
		group_num = stoi(num_str);
		//找出当前用户
		for (auto i : name_sock_map)
		{
			if (i.second == conn)
			{
				login_name = i.first;
				break;
			}
		}
		cout<<"用户"<<login_name<<"绑定的群号为:"<<num_str<<endl;
		pthread_mutex_lock(&group_mutx);	//加锁
		group_map[group_num].insert(conn);
		pthread_mutex_unlock(&group_mutx);	//解锁
	}
	//广播群聊消息
	else if (str.find("gr_message:") != str.npos)
	{
		cout<<"广播群聊信息方法\n";
		for (auto i : name_sock_map)
		{
			if (i.second == conn)
			{
				login_name = i.first;
				break;
			}
		}
		for (auto i : group_map)
		{
			if (i.second.find(conn) != i.second.end())
			{
				group_num = i.first;
				break;
			}
		}
		string send_str(str);
		send_str = send_str.substr(11);
		send_str = "[" + login_name + "]:" + send_str;
		cout<<"群聊信息："<<send_str<<endl;
		for (auto i:group_map[group_num])
		{
			if (i != conn)
				send(i, send_str.c_str(), send_str.length(), 0);
		}
	}

	//线程工作完毕后重新注册事件
	epoll_event event;
	event.data.fd=conn;
	event.events=EPOLLIN|EPOLLET|EPOLLONESHOT;
	epoll_ctl(epollfd,EPOLL_CTL_MOD,conn,&event);

	mysql_close(con);
	if(!redis_target->err)
		redisFree(redis_target);

	//更新
	get<0>(info) = if_login;
	get<1>(info) = login_name;
	get<2>(info) = target_name;
	get<3>(info) = target_conn;
	get<4>(info) = group_num;

}

//将文件描述符置为非阻塞状态
void server::setnonblocking(int sock)
{
	int opts;
	//fcntl可改变已打开的文件性质 GETFL：取得文件描述符的状态标志
	opts = fcntl(sock, F_GETFL);
	if (opts < 0)
	{
		perror("fcntl(sock, GETFL");
		exit(1);
	}
	//F_SETFL 设置文件描述符状态标志
	opts = opts|O_NONBLOCK;
	if (fcntl(sock, F_SETFL, opts) < 0)
	{
		perror("fcntl(sock, F_SETFL, opts)");
		exit(1);
	}
}
