#include "client.h"



client::client(int port, string ip):server_port(port),server_ip(ip){}
client::~client()
{
	close(sock);
}

void client::run()
{
	sock = socket(AF_INET,SOCK_STREAM,0);

	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(server_port);
	servaddr.sin_addr.s_addr = inet_addr(server_ip.c_str());

	if(connect(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		perror("connect");
		exit(1);
	}

	cout<<"成功连接服务器\n";

	HandleClient(sock);

	thread send_t(SendMsg, sock), recv_t(RecvMsg, sock);
	send_t.join();
	cout<<"发送线程已结束\n";
	recv_t.join();
	cout<<"接收线程已结束\n";

	return;

}

//"static" is not needed
void client::SendMsg(int conn)
{
	//char sendbuf[1024];
	while(1)
	{
		/*
		memset(sendbuf, 0, sizeof(sendbuf));
		cin>>sendbuf;
		int ret=send(conn, sendbuf, strlen(sendbuf), 0);
		//ends when input "exit" or closed on the other side.
		if(strcmp(sendbuf, "exit")==0 || ret<=0)
			break;
		*/
		string str;
		cin>>str;
		//send msg
		str = "content:" + str;
		int ret = send(conn, str.c_str(), str.length(), 0);
		if(str == "conntent:exit"||ret <= 0)
			break;
	}
}

//"static" is not needed
void client::RecvMsg(int conn)
{
	char buffer[1024];
	while(1)
	{
		memset(buffer, 0, sizeof(buffer));
		int len = recv(conn, buffer, sizeof(buffer), 0);
		if (len <= 0)
			break;
		cout<<buffer<<endl;
	}
}

void client::HandleClient(int conn)
{
	int choice;
	string name, pass, pass1;
	bool if_login = false;		//记录是否已登录
	string login_name;		//记录成功登录的用户名

	cout<<"--------------------\n";
	cout<<"|                  |\n";
	cout<<"|  请输入你的选择：|\n";
	cout<<"       0:退出       \n";
	cout<<"       1:登录       \n";
	cout<<"       2:注册       \n";
	cout<<"|                  |\n";
	cout<<"--------------------\n\n";

	//处理各种事物
	while(1)
	{
		if(if_login)
			break;
		cin>>choice;
		if(choice == 0)
		{
			break;
		}
		else if (choice == 1 && !if_login)
		{
			while(1)
			{
				cout<<"用户名：";
				cin>>name;
				cout<<"密码：";
				cin>>pass;
				//格式化：
				string str = "login"+name;
				str += "pass:";
				str +=pass;
				send(sock, str.c_str(), str.length(), 0);  //send info of login
				char buffer[1024];
				memset(buffer, 0, sizeof(buffer));
				recv(sock, buffer, sizeof(buffer), 0);     //get the response
				string recv_str(buffer);
				if(recv_str.substr(0, 2) == "ok")
				{
					if_login = true;
					login_name = name;
					cout<<"登录成功！\n\n";
					break;
				}
				else
					cout<<"用户名或密码错误！\n\n";
			}
		}
		//register
		else if (choice == 2)
		{
			cout<<"注册的用户名：";
			cin>>name;
			while(1)
			{
				cout<<"密码：";
				cin>>pass;
				cout<<"请确认密码：";
				cin>>pass1;
				if(pass == pass1)
					break;
				else
					cout<<"两次密码输入不一致！\n\n";
			}
		}
		name = "name:" + name;
		pass = "pass:" + pass;
		string str = name + pass;
		send(conn, str.c_str(), str.length(), 0);
		cout<<"注册成功！\n";
		cout<<"\n继续输入你的选择：";
	}

	//登录成功
	while(if_login && 1)
	{
		if(if_login)
		{

			system("clear");//清空终端d
			cout<<"        欢迎回来,"<<login_name<<endl;
			cout<<" -------------------------------------------\n";
			cout<<"|                                           |\n";
			cout<<"|          请选择你要的选项：               |\n";
			cout<<"|              0:退出                       |\n";
			cout<<"|              1:发起单独聊天               |\n";
			cout<<"|              2:发起群聊                   |\n";
			cout<<"|                                           |\n";
			cout<<" ------------------------------------------- \n\n";
		}
		cin>>choice;
		if(choice == 0)
			break;
		//私聊
		if (choice == 1)
		{
			cout<<"请输入对方的用户名：";
			string target_name, content;
			cin>>target_name;
			//向服务器发送目标用户 源用户
			string sendstr("target:"+target_name+"from:"+login_name);
			send(sock, sendstr.c_str(), sendstr.length(), 0);
			cout<<"请输入你向说的话（输入exit退出）：\n";
			thread t1(client::SendMsg, conn);	//发送进程
			thread t2(client::RecvMsg, conn);	//接收进程
			t1.join();
			t2.join();
		}
	}

	close(sock);
}


