#include "server.h"

int main(int argc, char* argv[])
{
	string ip = "127.0.0.1";
	int port = 8899;
	
	if (argv[1] != nullptr && argv[2] != nullptr)
	{
		ip = argv[1];
		port = atoi(argv[2]);
	}
	server serv(port, ip);        //创建实例，传入端口号和ip作为构造函数参数
	serv.run();                            //启动
}
