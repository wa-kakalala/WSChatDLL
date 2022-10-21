# include <winsock.h>
# include <conio.h>
# include "Chat_Server.h"
# include <stdio.h>
# include <string.h>
# pragma comment(lib,"wsock32.lib")

socket_list sock_list;
Message recv_msg;
Message send_msg;

/*����ɣ������Խ������ݵĴ�������֪�����֣����к�����ע��ûд�������Ĺ���û�⣬����֮����ɸ�dll���̣�д�׽���Ҳ��û�мӣ�*/

int Init(void (*f) (const char* msg, int msglen)) {
	recv_msg.len = strlen(recv_msg.data);
	f(recv_msg.data, recv_msg.len);
	recv_msg.len = 0;
	memset(recv_msg.data, 0, RECVDATA_SIZE);
	return 0;
}

void Defer() {

}

//�����������������Ȼ����ʾ������Init������ֵȫ������-1���أ�����C#���½��߳�
int  ServerStart(const char* ip, unsigned short* port, const char* name, int mode) {
	/*Para Init*/
	WSADATA wsa;
	SOCKET s, sock_acpt;
	struct sockaddr_in ser_addr, remote_addr;
	fd_set readfds, writefds, exceptfds;
	char buf[RECVBUF_SIZE];
	
	timeval timeout;
	unsigned long arg = 1;
	
	int retval,counter,len;//variaty always used
	len = sizeof(remote_addr);

	/*Init resources of socket*/
	if(WSAStartup(0x101,&wsa)!=0)//����0
		return -1;
	
	InitList(&sock_list);
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_addr.S_un.S_addr = inet_addr(ip); // ��λ�ֽڵ�λ��ַ����λ�ֽڸ�λ��ַ
	ser_addr.sin_port = htons(*port);

	ioctlsocket(sock_list.listen_sock_t, FIONBIO, &arg);

	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	//TCP socket init
	sock_list.listen_sock_t = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_list.listen_sock_t == INVALID_SOCKET || sock_list.listen_sock_t == SOCKET_ERROR)
	{
		printf("WSAGetLastError:%d", WSAGetLastError());
		return -1;
	}
	if (bind(sock_list.listen_sock_t, (sockaddr*)&ser_addr, sizeof(ser_addr) == SOCKET_ERROR))
	{

		printf("WSAGetLastError:%d", WSAGetLastError());
		return -1;
	}
	listen(sock_list.listen_sock_t, BACK_LOG_VAL);

	//UDP socket init
	sock_list.listen_sock_u = socket(AF_INET, SOCK_DGRAM, 0);
	if (bind(sock_list.listen_sock_u, (sockaddr*)&ser_addr, sizeof(ser_addr) == SOCKET_ERROR))
	{

		printf("WSAGetLastError:%d", WSAGetLastError());
		return -1;
	}
					   
	/*run server*/
	while (1)
	{
		//Select
		MakeFdlist(&sock_list, &readfds);
		if (select(0, &readfds, &writefds, &exceptfds, NULL) <= 0) {
			printf("WSAGetLastError:%d", WSAGetLastError());
			return -1;
		}

		//UDP receive
		if (FD_ISSET(sock_list.listen_sock_u, &readfds)) {
			retval = recvfrom(sock_list.listen_sock_u, buf, RECVBUF_SIZE, 0, (struct sockaddr*)&remote_addr, &len);
			if (retval < 0) return -1;
			buf[retval] = 0;
			strcpy_s(recv_msg.data,strlen(buf)+1,buf);
			printf("->%s\n", buf);
			memset(buf, 0, RECVBUF_SIZE);
		}

		//TCP receive:whether have new connect established
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
					if (retval == EWOULDBLOCK || retval == EAGAIN)
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
				printf("->%s\n", buf);
				memset(buf, 0, RECVBUF_SIZE);

			}
		}


		FD_ZERO(&readfds);
		//FD_ZERO(&writefds);
		//FD_ZERO(&exceptfds);

	}
}



int  ServerClose() {
	//�ͷ���Դ
	closesocket(sock_list.listen_sock_t);
	closesocket(sock_list.listen_sock_u);
	WSACleanup();
	return 0;
}

//Message send finished by client. Function useless
int MessageSend(char *buf) {
	return 0;
}

//Deal with received message
int RecvMessageProcess(char *buf) {
	return 0;
}

//Init socket list
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
//Insert socket 2 TCP management list
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
//Delete socket 2 TCP management list
void DeleteList(SOCKET s, socket_list* list) {
	int counter;
	for (counter = 0; counter < 64; counter++) {
		if (list->sock_list[counter] == s) {
			list->sock_list[counter] = INVALID_SOCKET;
			list->num += 1;
			break;
		}
	}
}
//Copy TCP management list 2 a fd status set
void MakeFdlist(socket_list* list, fd_set* fd_list) {
	int i;
	FD_SET(list->listen_sock_t, fd_list); //��������׽���
	FD_SET(list->listen_sock_u, fd_list);
	for (i = 0; i < 64; i++)	//�����׽����б�
		if (list->sock_list[i] != INVALID_SOCKET)
			FD_SET(list->sock_list[i], fd_list);
}


/*Function Test : The function below is useless, please ignore*/
void show(struct socket_list* list)
{
	printf("%d left in the array\n", list->num);
}
int test_mainsock(struct socket_list* list)
{
	int counter,retval;

	InitList(list);	//���ʼ������
	list->listen_sock_t = 100;
	counter = 15;
	show(list);
	do {
		SOCKET s = counter;
		InsertList(s, list);
	} while (--counter);	//����뺯��
	show(list);
	do {
		DeleteList(counter, list);	//��ɾ������
	} while ((counter++) != 14);
	show(list);
	return 1;
}
int message_save(char* buf) {
	return 0;
}
