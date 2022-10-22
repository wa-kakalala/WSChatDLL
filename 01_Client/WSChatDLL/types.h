#pragma once

#define CHAT_MODE_UDP (1)
#define CHAT_MODE_TCP (2)

typedef struct Message {
	int  type;
	int  len;
	char data[512];
}Message;

typedef enum MesType {
	MES_PING_REQ,
	MES_PING_REP
}MesType;

