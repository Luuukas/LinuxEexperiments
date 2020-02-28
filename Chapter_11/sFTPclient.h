// gcc sFTPclient.c sFTPclient.h sFTPfns.c sFTPfns.h -o Client -lpthread
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include "sFTPfns.h"
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "sFTPfile.h"
#include "sFTPpthread.h"

int check_format(char *command, char *server, char *user, char *passwd, int *port);

typedef int SOCKET;
#define INVALID_SOCKET -1
#define h_addr h_addr_list[0]
#define oops(m,x) { perror(m); exit(x); }

SOCKET FTP_RunDataListener(int* nPort, in_addr_t serverInAddr);
SOCKET FTP_Accept(SOCKET nSocket, in_addr_t *ptClientIP);
int FTP_CloseConnection(SOCKET nSocket);

typedef struct PORT_Accept_arg{
    int m_dSocket;
    in_addr_t *m_ptServerAddress;
    int *m_pnServerSocket;
}PORT_Accept_arg;

void portToStr(char sPort[], int dPort);
int make_stream_client_socket();
void *PORT_Accept(void *arg);
// int make_internet_address(char *hostname, int port, struct sockaddr_in *addrp);