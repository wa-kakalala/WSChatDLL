#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include "pch.h"
#include "types.h"
#include "pch.h"
#include <stdio.h>
#include <stdlib.h>


#pragma comment(lib,"ws2_32.lib")
#define BUF_SIZE (1024)

// TCP 信息
UserInfo ServerInfo = { 0 };
// UDP 信息
UserInfo ClientInfo = { 0 };
// tfd 和 ufd
SocketInfo ServerSocket = { 0 };
// 客户端 tfd 和 ufd
SocketInfo ClientSocket = { 0 };
// 回调用的信息
UploadInfo uploadinfo = { NULL };
// 存储所有的套接口
FdArray sfdarr = { 0 };

// 客户端用来记录上一次连接
LastConnect lastconn = { NULL,0,0 };
// read_fd, write_fd, except_fd;
FDS fds = { 0 };
// 要发送的信息
SendInfo sendinfo= {0};


// 缓存区
char buf[BUF_SIZE] = { 0 };


int newClient(SOCKET lstn_soc);
int recvData(SOCKET fd, int i);
int occurException(int i);
int sendData(int i);


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

/**
* 初始化，完成socket系统库的加载
* @return  0 success
*         -1 failed
*/
int  Init(void (*f) (const char* msg, int msglen)){
	if (WSA_Init() < 0) return -1;
	uploadinfo.f = f;
	sendinfo.maxindex = -1;
	return 0;
}

/**
* 卸载socket系统库
*/
void Defer() {
	WSACleanup();
}

int ServerStart(const char* ip, unsigned short port, const char* name, int mode) {
	strcpy(ServerInfo.ip, ip);
	ServerInfo.port = port;
	strcpy(ServerInfo.name, name);
	// 暂时不管mode
	struct sockaddr_in srv_addr;
	SOCKET client_sock;
	int result,cnt,i,addr_len = sizeof(srv_addr);
	ServerSocket.tfd = socket(AF_INET, SOCK_STREAM, 0);/* 基于TCP socket */
	/* 消息发送协议服务器地址 */
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(ServerInfo.port);
	srv_addr.sin_addr.s_addr = inet_addr(ServerInfo.ip);
	result = bind(ServerSocket.tfd, (struct sockaddr*)&srv_addr, addr_len);
	listen(ServerSocket.tfd, SOMAXCONN);
	getsockname(ServerSocket.tfd, (struct sockaddr*)&srv_addr, &addr_len);
	ServerInfo.port = ntohs(srv_addr.sin_port);
	ServerSocket.ufd = socket(AF_INET, SOCK_DGRAM, 0); /* 基于UDP socket */
	srv_addr.sin_port = htons(ServerInfo.port);
	bind(ServerSocket.ufd, (struct sockaddr*)&srv_addr, addr_len);
	//*port = ServerInfo.port;
	FD_ZERO(&fds.read_set);
	FD_ZERO(&fds.write_set);
	FD_ZERO(&fds.except_set);

	FD_SET(ServerSocket.tfd, &fds.read_set);
	FD_SET(ServerSocket.ufd, &fds.read_set);
	sfdarr.fdarr[0] = ServerSocket.ufd;
	sfdarr.maxindex = 0;
	struct timeval timout = { 0,100000 };
	while (1) {
		cnt = select(0, &fds.read_set, &fds.write_set, &fds.except_set, &timout);
		if (fds.read_set.fd_count != 0) {
			if (FD_ISSET(ServerSocket.tfd, &fds.read_set)) {  /* 检查TCP的侦听socket */
				newClient(ServerSocket.tfd);
			}
			// 处理接收
			for (int i = 0; i <= sfdarr.maxindex; i++) {
				client_sock = sfdarr.fdarr[i];
				if (!FD_ISSET(client_sock, &fds.read_set))
					continue;
				result = recvData(client_sock, i);
			}
		}
		if (fds.write_set.fd_count != 0) {
			// 处理发送 
			for (int i = 0; i <= sendinfo.maxindex; i++) {
				client_sock = sendinfo.senddata[i].fd;
				if (!FD_ISSET(client_sock, &fds.write_set))
					continue;
				result = sendData(i);
			}
			// 清除发送
			for (int i = 0; i <= sendinfo.maxindex; i++) {
				if (sendinfo.senddata[i].fd == 0) {
					free(sendinfo.senddata[i].data);
					sendinfo.senddata[i] = sendinfo.senddata[sendinfo.maxindex--];
				}
			}
		}
		
		if (fds.except_set.fd_count != 0) {
			// 处理异常 --> 主要是tcp客户端断开连接
			for (int i = 1; i <= sfdarr.maxindex; i++) {
				client_sock = sfdarr.fdarr[i];
				if (!FD_ISSET(client_sock, &fds.except_set))
					continue;
				result = occurException(i);
			}
		}
		
		// 更新
		FD_ZERO(&fds.read_set);
		FD_ZERO(&fds.write_set);
		FD_ZERO(&fds.except_set);
		FD_SET(ServerSocket.tfd, &fds.read_set);
		for (int i = 0; i <= sfdarr.maxindex; i++) {
			FD_SET(sfdarr.fdarr[i], &fds.read_set);
			if (i == 0) continue;
			FD_SET(sfdarr.fdarr[i], &fds.except_set);
		}

		for (int i = 0; i <= sendinfo.maxindex; i++) {
			FD_SET(sendinfo.senddata[i].fd, &fds.write_set);
		}

	}
}
/**
* 接收客户端的连接请求
* @param lstn_soc 监听套接口
* @param soc_set 新接受的套接口需要加入的集合
* @return success: 新接受的套接口  fail: INVALID SOCKET
*/
int newClient(SOCKET lstn_soc){
	int i;
	struct sockaddr_in faddr;
	int addr_len = sizeof(faddr);
	SOCKET acpt_soc;
	acpt_soc = accept(lstn_soc, (struct sockaddr*)&faddr, &addr_len);
	if (acpt_soc == INVALID_SOCKET)
		return INVALID_SOCKET;
	
	for (i = 0; i < FD_SETSIZE; i++)
	{
		if (sfdarr.fdarr[i] == 0)
		{
			sfdarr.fdarr[i] = acpt_soc;
			break;
		}
	}
	if (i == FD_SETSIZE) /* 已经满了 */
	{
		closesocket(acpt_soc);
		return INVALID_SOCKET;
	}
	if (i > sfdarr.maxindex)
		sfdarr.maxindex = i;
	return acpt_soc;
}


int recvData(SOCKET fd, int i) {
	int result = 0;
	if (i == 0) /* 基于UDP服务的socket */
		result = recvfrom(fd, buf, BUF_SIZE, 0, NULL,NULL);
	else
		result = recv(fd, buf, BUF_SIZE, 0); /* 基于TCP服务的socket */
	Message* msg = (Message*)buf;
	uploadinfo.f(buf + sizeof(Message), msg -> len);
	return result;
}

int occurException(int i) {
	closesocket(sfdarr.fdarr[i]);
	sfdarr.fdarr[i] = sfdarr.fdarr[sfdarr.maxindex--];
	sfdarr.fdarr[sfdarr.maxindex] = 0;
	return 0;
}

int sendData(int i) {
	int result = 0;
	Message msg = { 0 };
	struct sockaddr_in cli_addr;
	int addr_len = sizeof(cli_addr);
	cli_addr.sin_family = AF_INET;
	cli_addr.sin_port = htons(ClientInfo.port);
	cli_addr.sin_addr.s_addr = inet_addr(ClientInfo.ip);
	if (sendinfo.senddata[i].fd == ServerSocket.ufd) {
		result = sendto(sendinfo.senddata[i].fd, sendinfo.senddata[i].data, sendinfo.senddata[i].len, 0,
			(struct sockaddr*)(&cli_addr), addr_len);
	}else {
		result = send(sendinfo.senddata[i].fd, sendinfo.senddata[i].data, sendinfo.senddata[i].len, 0); /* 基于TCP的服务器 */
	}
	
	sendinfo.senddata[i].fd = 0;
	return result;
}


int  ClientConnect(const char* ip, unsigned short port, int mode) {
	strcpy(ClientInfo.ip, ip);
	ClientInfo.port = port;
	ClientInfo.mode = mode;
	if (mode == CHAT_MODE_UDP) {
		ClientSocket.ufd = ServerSocket.ufd;
		return 0;
	}
	if (lastconn.ip != NULL) {
		if (strcmp(ip,lastconn.ip) == 0 && port == lastconn.port) {
			return 0;
		}else {
			closesocket(lastconn.fd);
		}
	}

	struct sockaddr_in srv_addr;   /* 服务端地址 */
	int addr_len = sizeof(srv_addr);
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(port);
	srv_addr.sin_addr.s_addr = inet_addr(ip);
	ClientSocket.tfd = socket(AF_INET, SOCK_STREAM, 0);
	connect(ClientSocket.tfd, (struct sockaddr*)&srv_addr, addr_len);
	return 0;
}

int  ClientSendMessage(const char* msg, int msglen) {
	char senderinfo[100] = { 0 };
	int senderinfolen = sprintf(senderinfo, "[%s:%d]'%s:", ServerInfo.ip, ServerInfo.port, ServerInfo.name);
	char* buf = (char *)malloc(sizeof(Message) + msglen  + senderinfolen + 1); // 末尾添加一个'\0'
	Message * msghder = (Message*)buf;
	msghder->seq = 1; // 暂时不用
	msghder->type = MSGTYPE_TEXT;
	msghder->codetype = MSGCODE_NULL;
	msghder->len = msglen + 1 + senderinfolen;
	memcpy(buf + sizeof(Message), senderinfo, senderinfolen);
	memcpy(buf + sizeof(Message) + senderinfolen, msg, msglen);
	*(buf + sizeof(Message) + msglen + senderinfolen) = '\0';
	SendData sd = {
		0,
		sizeof(Message) + msghder->len,
		buf,
	};
	switch (ClientInfo.mode) {
	case CHAT_MODE_UDP:
		sd.fd = ClientSocket.ufd;
		break;
	case CHAT_MODE_TCP:
		sd.fd = ClientSocket.tfd;
		break;
	}

	sendinfo.senddata[++sendinfo.maxindex] = sd;
	return 0;
}

int  ServerClose() {
	for (int i = 0; i <= sfdarr.maxindex; i++) {
		closesocket(sfdarr.fdarr[i]);
	}
	closesocket(ServerSocket.tfd);
	return 0;
}

int  ClientClose() {
	return 0;
}



