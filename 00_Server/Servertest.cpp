# include<iostream>
# include "Chat_Server.h"



int main()
{
	char addr[] = "127.0.0.1";
	unsigned short port = 56555;
	char name[] = "Server";
	int mode = 1;
	ServerStart(addr,&port,name,mode);
}