#pragma once

#include <winsock2.h>

#define CHAT_MODE_UDP (1)
#define CHAT_MODE_TCP (2)

typedef struct Message {
	unsigned char seq;		// 序列号
	unsigned char type;		// 消息类型
	unsigned char codetype; // 编码类型
	unsigned int  len;		// 消息长度
}Message;

typedef enum MesType {
	MSGTYPE_TEXT,
	MSGTYPE_PICTURE,
}MesType;

typedef enum CodeType {
	MSGCODE_NULL,
}CodeType;

typedef struct UserInfo {
	char ip[20];
	unsigned short port;
	char name[20];
	int mode;
}UserInfo;

typedef struct SocketInfo {
	int tfd;
	int ufd;
}SocketInfo;

typedef struct UploadInfo {
	void (*f) (const char* msg, int msglen);
}UploadInfo;

typedef struct FdArray {
	int fdarr[FD_SETSIZE];  // 存储套接口信息
	int maxindex; // 最大的下标
}SfdArray;

typedef struct LastConnect {
	char ip[20];
	unsigned short port;
	int fd;
}LastConnect;

typedef struct FDS {
	fd_set read_set;
	fd_set write_set;
	fd_set except_set;
}FDS;


typedef struct SendData {
	SOCKET fd;        // 发送的套接字
	unsigned int len; // 发送的数据长度
	char* data;       // 发送的数据
}SendData;

typedef struct SendInfo {
	SendData senddata[FD_SETSIZE];
	int maxindex;
}SendInfo;

