# include "Chat_Server.h"


socket_list sock_list;
Message recv_msg;
Message send_msg;
//char key_input_buf[SENDDATA_SIZE];
//std::mutex input_buf_lock;


void (* F) (const char* msg, int msglen);

int Init(void (*f) (const char* msg, int msglen)) {
	/*recv_msg.len = strlen(recv_msg.data);
	f(recv_msg.data, recv_msg.len);
	recv_msg.len = 0;
	memset(recv_msg.data, 0, RECVDATA_SIZE);
	return 0;*/
	F = f;
	return 0;
}

void Defer() {

}

/*
* @brief Usage :  start TCP and UDP Server
* @param ip: server ip 
* @param port: server 's bind port
* @param mode:1(UDP) 2(TCP)
* 
*/
int  ServerStart(const char* ip, unsigned short* port, const char* name, int mode) {
	/*WSA Error File*/
	FILE* fp = NULL;
	errno_t err = fopen_s(&fp,"WSAGetLastError.txt", "w+");

	/*Para Init*/
	WSADATA wsa;
	SOCKET s, sock_acpt;
	struct sockaddr_in ser_addr, remote_addr;
	fd_set readfds, writefds, exceptfds;
	char buf[RECVBUF_SIZE];
	char send_buf[RECVBUF_SIZE];
	
	timeval timeout;
	unsigned long arg = 1;
	
	int retval,counter,len;//variaty always used
	len = sizeof(remote_addr);

	/*Init resources of socket*/
	if(WSAStartup(0x101,&wsa)!=0)//返回0
		return -1;
	
	InitList(&sock_list);
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);
	
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_addr.S_un.S_addr = inet_addr(ip); // 高位字节低位地址，低位字节高位地址
	ser_addr.sin_port = htons(*port);

	ioctlsocket(sock_list.listen_sock_t, FIONBIO, &arg);
	//ioctlsocket(sock_list.listen_sock_u, FIONBIO, &arg);

	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	//TCP socket init
	sock_list.listen_sock_t = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_list.listen_sock_t == INVALID_SOCKET || sock_list.listen_sock_t == SOCKET_ERROR)
	{
		fprintf(fp, "Line: %d WSAGetLastError : %d\n",__LINE__,WSAGetLastError());
		return -1;
	}
	if (bind(sock_list.listen_sock_t, (sockaddr*)&ser_addr, sizeof(ser_addr))==SOCKET_ERROR)
	{

		fprintf(fp, "Line: %d WSAGetLastError : %d\n", __LINE__, WSAGetLastError());
		return -1;
	}
	listen(sock_list.listen_sock_t, BACK_LOG_VAL);

	//UDP socket init
	sock_list.listen_sock_u = socket(AF_INET, SOCK_DGRAM, 0);
	if (bind(sock_list.listen_sock_u, (sockaddr*)&ser_addr, sizeof(ser_addr))==SOCKET_ERROR)
	{

		fprintf(fp, "Line: %d WSAGetLastError : %d\n", __LINE__, WSAGetLastError());
		return -1;
	}
					   
	/*run server*/
	while (1)
	{
		/*1 Select*/
		MakeFdlist(&sock_list, &readfds);
		//MakeFdlist(&sock_list, &writefds);
		if (select(0, &readfds, &writefds, &exceptfds, &timeout) < 0) {
			fprintf(fp, "Line: %d WSAGetLastError : %d\n", __LINE__, WSAGetLastError());
			return -1;
		}

		/*2 UDP receive*/
		if (FD_ISSET(sock_list.listen_sock_u, &readfds)) {
			retval = recvfrom(sock_list.listen_sock_u, buf, RECVBUF_SIZE, 0, (struct sockaddr*)&remote_addr, &len);
			fprintf(fp, "Line: %d WSAGetLastError : %d\n", __LINE__, WSAGetLastError());
			if (retval == -1)
				return -1;
			buf[retval] = 0;
			strcpy_s(recv_msg.data,strlen(buf)+1,buf);
			recv_msg.len = strlen(recv_msg.data) + 1;
			//F(recv_msg.data, strlen(recv_msg.data) + 1);
			fprintf(fp, "Line: %d UDP Receive:%s\n", __LINE__, recv_msg.data);
			memset(buf, 0, RECVBUF_SIZE);

		}

		//UDP send
		/*if (FD_ISSET(sock_list.listen_sock_u,&writefds))
		{
			std::lock_guard<std::mutex> lock(input_buf_lock);
			strcpy_s(send_msg.data, strlen(key_input_buf) + 1, key_input_buf);
			memset(key_input_buf, 0, SENDDATA_SIZE);
			sprintf_s(send_buf, "%s:\n%s\n", send_msg.name, send_msg.data);
			//retval = sendto(s, send_buf, strlen(send_msg.name) + strlen(send_msg.data), 0,);?咋发啊，发给谁
		}*/

		/*3 TCP receive:whether have new connect established*/
		if (FD_ISSET(sock_list.listen_sock_t, &readfds)) {
			sock_acpt = accept(sock_list.listen_sock_t, (sockaddr*)&remote_addr, &len);
			if (sock_acpt == SOCKET_ERROR || sock_acpt == INVALID_SOCKET)
			{
				printf("WSAGetLastError:%d", WSAGetLastError());
				return -1;
			}
			else InsertList(sock_acpt, &sock_list);
		}
		for (counter = 0; counter < SOCK_LIST_SIZE; counter++)		//check TCP in connect list
		{
			if (sock_list.sock_list[counter] == INVALID_SOCKET)
				continue;
			s = sock_list.sock_list[counter];
			if(FD_ISSET(s,&readfds)){
				retval = recv(s, buf, RECVBUF_SIZE, 0);
				if (retval == 0)	//close connection normally
				{
					closesocket(s);
					DeleteList(s, &sock_list);
					printf("Close socket normally");
					continue;
				}
				else if (retval == SOCKET_ERROR)
				{
					if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EAGAIN)
						continue;
					/*if (retval == EINTR)
						retval = recv(s, buf, RECVBUF_SIZE, 0);*/
					closesocket(s);
					DeleteList(s, &sock_list);
					printf("WSAGetLastError:%d", WSAGetLastError());
					continue;
				}
				buf[retval] = 0;
				strcpy_s(recv_msg.data,strlen(buf)+1,buf);
				recv_msg.len = strlen(recv_msg.data) + 1;
				fprintf(fp, "Line: %d TDP Receive:%s\n", __LINE__, recv_msg.data);
				//F(recv_msg.data, strlen(recv_msg.data) + 1);
				//printf("->%s\n", buf);
				memset(buf, 0, RECVBUF_SIZE);
			}

			//TCP send
			/*if (FD_ISSET(s, &writefds)) {//lock_guard and unique guard will block
				//lock write socket?  key input buf
				std::lock_guard<std::mutex> lock(input_buf_lock);
				strcpy_s(send_msg.data, strlen(key_input_buf) + 1, key_input_buf);
				memset(key_input_buf, 0, SENDDATA_SIZE);
				sprintf_s(send_buf, "%s:\n%s\n", send_msg.name, send_msg.data);
				retval = send(s, send_buf, strlen(send_msg.name) + strlen(send_msg.data), 0);
				}
			}*/
		}
		FD_ZERO(&readfds);
		//FD_ZERO(&writefds);
		//FD_ZERO(&exceptfds);
	}
	fclose(fp);
}


// @brief Usage :  close TCP and UDP Server
int  ServerClose() {
//释放资源
	closesocket(sock_list.listen_sock_t);
	closesocket(sock_list.listen_sock_u);
	WSACleanup();
	return 0;
}



// @brief Usage : Init socket list
void InitList(socket_list* list) {
	int counter = 0;
	list->listen_sock_t = INVALID_SOCKET;
	list->listen_sock_u = INVALID_SOCKET;
	list->num = 0;
	for (counter = 0; counter < SOCK_LIST_SIZE; counter++)
	{
		list->sock_list[counter] = INVALID_SOCKET;
	}
}

// @brief Usage : Insert socket 2 TCP management list
void InsertList(SOCKET s, socket_list* list) {
	int counter;
	for (counter = 0; counter < 64; counter++) {
		if (list->sock_list[counter] == INVALID_SOCKET) {
			list->sock_list[counter] = s;
			list->num += 1;
			break;
		}
	}
}

// @brief Usage : Delete socket 2 TCP management list
void DeleteList(SOCKET s, socket_list* list) {
	int counter;
	for (counter = 0; counter < 64; counter++) {
		if (list->sock_list[counter] == s) {
			list->sock_list[counter] = INVALID_SOCKET;
			list->num -= 1;
			break;
		}
	}
}

// @brief Usage : Copy TCP management list 2 a fd status set
void MakeFdlist(socket_list* list, fd_set *fd_list) {
	int i;
	FD_SET(list->listen_sock_t, fd_list); //加入监听套接字
	FD_SET(list->listen_sock_u, fd_list);
	for (i = 0; i < 64; i++)	//加入套接字列表
		if (list->sock_list[i] != INVALID_SOCKET)
			FD_SET(list->sock_list[i], fd_list);
}
