// pch.cpp: 与预编译标头对应的源文件
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include "pch.h"
#include <stdio.h>
#include "types.h"
#include "pch.h"
#include <winsock2.h>
#include <time.h>
#pragma comment(lib,"ws2_32.lib")


void (* F) (const char* msg, int msglen);
const char* IP = NULL;
unsigned short PORT = 0;
int MODE = -1;

SOCKET CFD = 0;


/**
* 初始化WSA
* 操作系统根据请求的Socket版本搜索相应的Socket库 --> 绑定到该应用程序
* @return -1 fail
		   0 success
*/
int WSA_Init() {
	WORD sockVersion = MAKEWORD(2, 2); // socket verison 
	WSADATA wsaData = { 0 };
	if (WSAStartup(sockVersion, &wsaData) != 0) {
		return -1;
	}
	return 0;
}

int Init(void (*f0) (const char* msg, int msglen)) {
	if (WSA_Init() < 0) return -1;
	F = f0;
	return 0;
}

int  ClientClose() {
	if (CFD) return -1;
	if (closesocket(CFD) == SOCKET_ERROR) return -1;
	return 0;
}

int ClientConnect(const char* ip, unsigned short port, int mode) {
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
		result = connect(CFD,(struct sockaddr*)&serv_addr, sizeof(serv_addr));
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
}


int  ClientSendMessage(const char* msg, int msglen) {
	Message message = {0};
	strcpy(message.data, msg);
	message.len = msglen;
	switch (MODE) {
	case CHAT_MODE_UDP:
		struct sockaddr_in serv_addr;  /* 服务器地址 */
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(PORT);
		serv_addr.sin_addr.s_addr = inet_addr(IP);;
		sendto(CFD, (char*)(&message), sizeof(int) * 2 + msglen, 0,
			(struct sockaddr*)&serv_addr, sizeof(serv_addr));
		break;
	case CHAT_MODE_TCP:
		// 可以直接发送，也可以先缓存放到select中
		send(CFD, (char*)(&message), sizeof(int) * 2 + msglen, 0);
		break;
	default:
		return -1;
	}

}

void Defer() {
	WSACleanup();
}
