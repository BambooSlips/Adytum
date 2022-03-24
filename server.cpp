#include "server.h"

unordered_map<int, set<int> >server::group_map;	//记录群号和套接字描述符集合
pthread_mutex_t server::group_mutx;		//互斥锁

vector<bool> server::sock_arr(1024, false);	//use command "ulimit -n" to check
unordered_map<string, int> server::name_sock_map;
pthread_mutex_t server::name_sock_mutx;
server::server(int port, string ip): server_port(port),server_ip(ip) 
{
	pthread_mutex_init(&name_sock_mutx, NULL);  //创建互斥锁
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
	tuple<bool, string, string, int, int> info; //if_login login_name target_name target_conn
	get<0>(info) = false;	//将if_login置false
	get<3>(info) = -1;	//target_conn置-1

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
}

void server::HandleRequest(int conn, string str, tuple<bool, string, string, int, int> &info)
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
		string target = str.substr(7, pos1-7), from = str.substr(pos1+4);
		target_name = target;
		if (name_sock_map.find(target) == name_sock_map.end())
			cout<<"源用户"<<login_name<<"，目标用户"<<target_name<<"尚未登录，无法发起私聊\n";
		else
		{
			cout<<"源用户"<<login_name<<"向目标用户"<<target_name<<"发起的私聊将建立";
			cout<<"，目标用户的套接字描述符为"<<name_sock_map[target]<<endl;
			target_conn = name_sock_map[target];
		}
	}
	//接收消息并转发
	else if(str.find("connect:") != str.npos)
	{
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
		string recv_str(str);
		string num_str = recv_str.substr(6);
		group_num = stoi(num_str);
		cout<<"用户"<<login_name<<"绑定的群号为:"<<num_str<<endl;
		pthread_mutex_lock(&group_mutx);	//加锁
		group_map[group_num].insert(conn);
		pthread_mutex_unlock(&group_mutx);	//解锁
	}
	//广播群聊消息
	else if (str.find("gr_message:") != str.npos)
	{
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

	//更新
	get<0>(info) = if_login;
	get<1>(info) = login_name;
	get<2>(info) = target_name;
	get<3>(info) = target_conn;
	get<4>(info) = group_num;

}
