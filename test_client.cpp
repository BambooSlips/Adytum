#include "client.h"

int main()
{
	client clnt(8899, "127.0.0.1");
	clnt.run();
}
