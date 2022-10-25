// Client.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
# include <stdio.h>
# include <string.h>
# include <mutex>
# pragma comment(lib,"wsock32.lib")
# include <winsock.h>
# include <conio.h>

#define CHAT_MODE_UDP (1)
#define CHAT_MODE_TCP (2)

typedef struct Message {
	int  len;
	char  name[32];
	char data[50];
}Message;

typedef enum MesType {
	MES_PING_REQ,
	MES_PING_REP
}MesType;


void (*F) (const char* msg, int msglen);
const char* IP = NULL;
unsigned short PORT = 0;
int MODE = -1;

SOCKET CFD = 0;


int WSA_Init() {
	WORD sockVersion = MAKEWORD(2, 2); // socket verison 
	WSADATA wsaData = { 0 };
	if (WSAStartup(sockVersion, &wsaData) != 0) {
		return -1;
	}
	return 0;
}


int ClientConnect(char* ip, unsigned short port, int mode) {
	MODE = mode;
	int result = 0;
	switch (mode) {
	case CHAT_MODE_UDP:
		IP = ip;
		PORT = port;
		CFD = socket(AF_INET, SOCK_DGRAM, 0);
		break;
	case CHAT_MODE_TCP:
		CFD = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in serv_addr;  /* 服务器地址 */
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(port);
		serv_addr.sin_addr.s_addr = inet_addr(ip);
		result = connect(CFD, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
		if (result == SOCKET_ERROR) /* 连接失败 */
		{
			closesocket(CFD);
			return -1;
		}
		break;
	default:
		return -1;
	}
	// 将CFD加入 Read_FD 等待接收数据
	return 0;
}

int  ClientSendMessage(char* msg, int msglen) {
	Message message = { 0 };
	strcpy_s(message.data, msg);
	message.len = msglen;
	int retval;
	switch (MODE) {
	case CHAT_MODE_UDP:
		struct sockaddr_in serv_addr;  /* 服务器地址 */
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(PORT);
		serv_addr.sin_addr.s_addr = inet_addr(IP);
		sendto(CFD, (char*)(&message.data), sizeof(int) * 2 + msglen, 0,
			(struct sockaddr*)&serv_addr, sizeof(serv_addr));
		retval = WSAGetLastError();
		break;
	case CHAT_MODE_TCP:
		// 可以直接发送，也可以先缓存放到select中
		send(CFD, (char*)(&message), sizeof(int) * 2 + msglen, 0);
		break;
	default:
		return -1;
	}
	return 0;
}

int main()
{
	char addr[] = "127.0.0.1";
	unsigned short port = 56555;
	char name[] = "Server";
	Message client_msg;
	strcpy_s(client_msg.name, "Client");
	strcpy_s(client_msg.data, "Hello, I am a client!");
	int mode = 2;
	int retval;

	retval = WSA_Init();
	retval = ClientConnect(addr, port, mode);
	retval = ClientSendMessage(client_msg.data, sizeof(char) * 512);

}