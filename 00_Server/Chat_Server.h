# include <stdio.h>
# include <string.h>
# include <mutex>
# pragma comment(lib,"wsock32.lib")
# include <winsock.h>
# include <conio.h>
# include <errno.h>

# define _CRT_SECURE_NO_WARNINGS

#define CHAT_MODE_UDP 1
#define CHAT_MODE_TCP 2
#define LISTEN_PORT_T 0x1234
#define LISTEN_PORT_U 0x1234
#define SOCK_LIST_SIZE 64

#define SENDDATA_SIZE 512   //Defines the buffer size for receiving keyboard data
#define RECVDATA_SIZE 512   //Defines the size of the message buffer to be fetched by the application layer
#define SENDBUF_SIZE 512     //Defines the size of the message buffer that will be sent
#define RECVBUF_SIZE 1024    //Define the buffer size for receiving remote data
#define MSG_DATA 1024

#define BACK_LOG_VAL 5        
//Server's socket list
typedef struct socket_list {
    SOCKET listen_sock_t;//refer to MSP,port is 18
    SOCKET listen_sock_u;
    int num;
    SOCKET sock_list[SOCK_LIST_SIZE];
}socket_list;


//Message format
typedef struct Message {
    int len;
    char name[32];
    char data[MSG_DATA];
}Message;

int  Init(void (*f) (const char* msg, int msglen));
int  ServerStart(const char* ip, unsigned short* port, const char* name, int mode);//Server Start
int  ServerClose();//Server Close

void InitList(socket_list* list);
void InsertList(SOCKET s, socket_list* list);
void DeleteList(SOCKET s, socket_list* list);
void MakeFdlist(socket_list* list, fd_set* fd_list);


//useless function£¬ignore
void show(struct socket_list* list);
int test_mainsock(struct socket_list* list);
int message_save(char* buf);
