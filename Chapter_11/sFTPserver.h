// gcc sFTPserver.c sFTPserver.h sFTPcnt.c sFTPcnt.h sFTPfns.c sFTPfns.h -o Server -lpthread
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#include "sFTPcnt.h"
#include "sFTPfns.h"
#include "sFTPfile.h"
#include "sFTPpthread.h"

#define COMMAND_BUFFER 256

typedef int SOCKET;
#define INVALID_SOCKET -1
#define oops(m,x) { perror(m); exit(x); }
SOCKET FTP_RunServer(int nPort);

SOCKET FTP_Accept(SOCKET nSocket, in_addr_t *ptClientIP);

int PerformRoutine(tPtclMsg tRecvMessage, tClientInfo *pClientInfo);

int FTP_CreateNewClientThread(SOCKET, in_addr_t);

char *FTP_GetIP(in_addr_t nConnectTo);

const int SPORT = 8080;
const int DPORT = 8082;

void *FTP_ServerThread(void *pParam);

int StartServer();

void *FTP_ClientThread(void *pParam);

void SignalHandler(int nSignalNo);

void TerminateProgram();

void TerminateCommand();

int checkuser(char *username, char *password);

SOCKET FTP_RunDataListener(int* nPort, in_addr_t serverInAddr);

int FTP_CloseConnection(SOCKET nSocket);

int StartScheduler();

typedef struct PORT_Accept_arg{
    int m_dSocket;
    in_addr_t *m_ptServerAddress;
    int *m_pnServerSocket;
}PORT_Accept_arg;

void *PORT_Accept(void *arg);

void portToStr(char sPort[], int dPort);