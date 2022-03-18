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
	char sendbuf[1024];
	while(1)
	{
		memset(sendbuf, 0, sizeof(sendbuf));
		cin>>sendbuf;
		int ret=send(conn, sendbuf, strlen(sendbuf), 0);
		//ends when input "exit" or closed on the other side.
		if(strcmp(sendbuf, "exit")==0 || ret<=0)
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
		if(len <= 0)
			break;
		cout<<"got a message from the server:"<<buffer<<endl;
	}
}

void client::HandleClient(int conn)
{
	int choice;
	string name, pass, pass1;

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
		cin>>choice;
		if(choice == 0)
		{
			break;
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
}


