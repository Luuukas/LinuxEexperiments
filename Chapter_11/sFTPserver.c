#include "sFTPserver.h"

SOCKET g_nServerSocket;
tMutex g_tCounterMutex;
tThread g_tFTP_ServerThread;
tThread g_tFTP_ClientThread;
int Counter = 0;

void *PORT_Accept(void *arg){
    PORT_Accept_arg *PAarg = (PORT_Accept_arg*)arg;
    *(PAarg->m_pnServerSocket) = FTP_Accept(PAarg->m_dSocket, PAarg->m_ptServerAddress);
    printf("Done. nServerSocket: %d\n",*(PAarg->m_pnServerSocket));
}

SOCKET FTP_RunServer(int nPort){
    SOCKET nServerSocket = INVALID_SOCKET;
    struct sockaddr_in tAddress;
    nServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    printf("1");
    if(nServerSocket==INVALID_SOCKET){
        return INVALID_SOCKET;
    }
    tAddress.sin_port = htons((unsigned short)nPort);
    tAddress.sin_family = AF_INET;
    tAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    printf("2");
    if(bind(nServerSocket, (const struct sockaddr *)&tAddress, sizeof(struct sockaddr_in))==INVALID_SOCKET){
        return -1;
    }
    printf("3");
    if(listen(nServerSocket, SOMAXCONN)==INVALID_SOCKET){
        return INVALID_SOCKET;
    }
    printf("4");
    return nServerSocket;
}

SOCKET FTP_RunDataListener(int* nPort, in_addr_t serverInAddr){
    SOCKET nServerSocket = INVALID_SOCKET;
    struct sockaddr_in tAddress;
    nServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(nServerSocket==INVALID_SOCKET){
        return INVALID_SOCKET;
    }
    tAddress.sin_port = htons((unsigned short)8082);    // 系统随机分配端口
    tAddress.sin_family = AF_INET;
    tAddress.sin_addr.s_addr = htonl(INADDR_ANY);    // 只允许指定客户端连接
    if(bind(nServerSocket, (const struct sockaddr *)&tAddress, sizeof(struct sockaddr_in))==INVALID_SOCKET){
        return -1;
    }
    if(listen(nServerSocket, SOMAXCONN)==INVALID_SOCKET){
        return INVALID_SOCKET;
    }
    *nPort = ntohs(tAddress.sin_port);    // 保存系统分配的端口号
    return nServerSocket;
}

SOCKET FTP_Accept(SOCKET nSocket, in_addr_t *ptClientIP){
    struct sockaddr_in tAddr_in;
    socklen_t nLength = sizeof(struct sockaddr_in);
    SOCKET nAccSocket = accept(nSocket, (struct sockaddr*)&tAddr_in, &nLength);

    if(nAccSocket<0){
        return INVALID_SOCKET;
    }

    if(ptClientIP){
        *ptClientIP = tAddr_in.sin_addr.s_addr;
    }
    return nAccSocket;
}

int checkuser(char *username, char *password){
    printf("%s %s\n", username, password);
    if(strcmp(username,"Luuukas")==0&&strcmp(password,"Chong516")==0)
        return 0;
    return -1;
}

void *FTP_ServerThread(void *pParam){
    g_nServerSocket = FTP_RunServer(SPORT);
    if(g_nServerSocket==-1){
        printf("Cannot start server. Terminated.\n");
        return (void *)-1;
    }
    printf("server launched. wait for users...\n");
    char user[20], passwd[20];
    for(;;){
        in_addr_t tClientAddress;
        SOCKET nClientSocket = FTP_Accept(g_nServerSocket, &tClientAddress);
        if(nClientSocket==INVALID_SOCKET){
            printf("Cannot accept new user. Terminated.\n");
            return (void *)-1;
        }
        tPtclMsg tRecvMessage;
        if(FTP_RecvMessage(nClientSocket, &tRecvMessage)==-1){
            printf("Failed to receive message from client.\n");
            return (void *)-1;
        }
        if(tRecvMessage.m_tHeader.m_nType!=FTP_USERAUTH){
            printf("Failed to authenticated user.\n");
            return (void *)-1;
        }
        // 用户端connect后的第一个数据必须为用户认证
        memcpy(user,tRecvMessage.m_pData,20);
        memcpy(passwd,tRecvMessage.m_pData+20,20);
        if(checkuser(user, passwd)!=-1){
            // LockMutex(&g_tCounterMutex);
            // Counter++;
            // UnlockMutex(&g_tCounterMutex);
            if(FTP_Response(nClientSocket, 200)==-1){
                printf("Failed to response code.\n");
                return (void *)-1;
            }
            if(FTP_CreateNewClientThread(nClientSocket, tClientAddress)==-1){
                printf("Cannot add new thread. Terminated.\n");
                return (void *)-1;
            }
        }else
        {
            printf("Invalid username or password!\n");
            if(FTP_Response(nClientSocket, 201)==-1){
                printf("Failed to response code.\n");
                return (void *)-1;
            }
        }
    }
    return (void *)0;
}

int StartServer(){
    StartThread(&g_tFTP_ServerThread, FTP_ServerThread, NULL);
    DetachThread(g_tFTP_ServerThread);
    return 0;
}

int FTP_CreateNewClientThread(SOCKET nSocket, in_addr_t tClientAddress){
    tClientInfo *pNewInfo = NULL;

    pNewInfo = (tClientInfo *)malloc(sizeof(tClientInfo));
    if(!pNewInfo){
        printf("Cannot alloc memory for tThreadInfo.\n");
        return -1;
    }
    pNewInfo->m_nSocket = nSocket;
    pNewInfo->m_tClientAddress = tClientAddress;
    StartThread(&g_tFTP_ClientThread, FTP_ClientThread, pNewInfo);
    return 0;
}

void *FTP_ClientThread(void *pParam){
    // 用专门的线程接收21端口上特定用户的数据
    tClientInfo *pClientInfo = (tClientInfo *)pParam;
    printf("Child Thread started.\n");
    // 连接21端口后的第一个数据应该是工作模式，如果是PORT还有用户端口号
    printf("5\n");
    tPtclMsg tRecvMessage;
    if(FTP_RecvMessage(pClientInfo->m_nSocket, &tRecvMessage)==-1){
        printf("Failed to receive first message from client.\n");
        return (void*)-1;
    }
    printf("6\n");
    // printf("%d\n",atoi(tRecvMessage.m_pData));
    // 增加在线用户计数
    LockMutex(&g_tCounterMutex);
    Counter++;
    UnlockMutex(&g_tCounterMutex);
    for(;;){    // 与客户的交互处理
        // 在performRoutine中等待新数据，分主动模式和被动模式进行处理
        if(PerformRoutine(tRecvMessage, pClientInfo)==-1) break;    // 根据客户交互信息类型，做相应处理
    }
    FTP_CloseConnection(pClientInfo->m_nSocket);
    printf("Child Thread ended.\n");
    return (void *)0;
}

void portToStr(char sPort[], int dPort){
    char tsPort[6];
    int i=0;
    while(dPort){
        tsPort[i++] = dPort%10 + '0';
        dPort /= 10;
    }
    for(int j=i-1;j>=0;j--){
        sPort[i-1-j] = tsPort[j];
    }
    sPort[i] = '\0';
}

int make_stream_client_socket(){
    return socket(AF_INET, SOCK_STREAM, 0);
}

int PerformRoutine(tPtclMsg tRecvMessage, tClientInfo *pClientInfo){
    printf("1");
    tPtclMsg nRecvMessage;
    if(FTP_RecvMessage(pClientInfo->m_nSocket, &nRecvMessage)==-1){
        printf("Failed to receive new message from client.\n");
        return -1;
    }
    printf("2");
    if(nRecvMessage.m_tHeader.m_nType==FTP_CD){

    }
    if(nRecvMessage.m_tHeader.m_nType==FTP_MKDIR){

    }
    if(nRecvMessage.m_tHeader.m_nType==FTP_RMDIR){

    }
    if(nRecvMessage.m_tHeader.m_nType==FTP_LS){

    }
    if(nRecvMessage.m_tHeader.m_nType==FTP_UPLOAD){
        char filename[30];
        memcpy(filename, nRecvMessage.m_pData, nRecvMessage.m_tHeader.m_nBytes);
        printf("filename: %s\n", filename);
        if(tRecvMessage.m_tHeader.m_nType==FTP_PORT){
            // 服务器20端口连接客户端的随机端口
            in_port_t dPort = (in_port_t)atol(tRecvMessage.m_pData);
            struct sockaddr_in client_addr;
            SOCKET dSocket;
            // SOCKET dSocket = FTP_PortConnect(pClientInfo->m_tClientAddress, dPort);
            {
                if((dSocket=make_stream_client_socket())==-1)    // 创建 TCP socket 失败
                    oops("cannot make socket",2);
                bzero(&client_addr,sizeof(client_addr)); // 初始化服务器地址
                client_addr.sin_family = AF_INET;
                client_addr.sin_port = htons(dPort);
                client_addr.sin_addr.s_addr = pClientInfo->m_tClientAddress;
                // inet_pton(AF_INET, server, &server_addr.sin_addr);

                struct sockaddr_in client;
                client.sin_family = AF_INET;
                client.sin_addr.s_addr = pClientInfo->m_tClientAddress;
                client.sin_port = DPORT;
                // 绑定本地20端口
                if(bind(dSocket, (struct sockaddr *)&client, sizeof(struct sockaddr_in))==INVALID_SOCKET){
                    printf("Failed to bind at PerformRoutine.\n");
                    return INVALID_SOCKET;
                }

                if(connect( dSocket, ( struct sockaddr* )&client_addr, sizeof( client_addr ) )==-1)
                    oops("cannot connect", 5);
            }
            if(dSocket==-1){
                printf("Failed to PORT connect client.\n");
                return -1;
            }
            // 用这个新的连接接收文件数据
            {
                tPtclMsg RecvMessage;
                FTP_RecvMessage(dSocket, &RecvMessage);
                ((char *)RecvMessage.m_pData)[RecvMessage.m_tHeader.m_nBytes] = '\0';
                printf("3");
                printf("filedata: \n%s\n", (char *)RecvMessage.m_pData);
                ASCII_write(filename, (char *)RecvMessage.m_pData);
            }
            // 关闭这个数据传输的连接
            FTP_CloseConnection(dSocket);
        }
        if(tRecvMessage.m_tHeader.m_nType==FTP_PASV){    
            // 服务器开启随机端口等待客户端连接
            int dPort;
            SOCKET dSocket = FTP_RunDataListener(&dPort, pClientInfo->m_tClientAddress);
            if(dSocket==-1){
                printf("Cannot start data listener.\n");
                return -1;
            }

            printf("dPort: %d\n", dPort);

            // 等待用户连接
            in_addr_t tClientAddress;
            SOCKET nClientSocket;
            PORT_Accept_arg PAarg;
            PAarg.m_dSocket = dSocket;
            PAarg.m_ptServerAddress = &tClientAddress;
            PAarg.m_pnServerSocket = &nClientSocket;
            tThread tt;
            StartThread(&tt, PORT_Accept, (void *)&PAarg);

            // PASV模式下用21端口告知用户连接服务器的一个随机端口号
            tPtclMsg pMsg;
            char sPort[6];
            portToStr(sPort, dPort);
            pMsg.m_tHeader.m_nType = FTP_PASV;
            pMsg.m_tHeader.m_nBytes = sizeof(sPort);
            pMsg.m_pData = sPort;
            FTP_SendMessage(pClientInfo->m_nSocket, &pMsg);
            printf("sPort: %s\n", sPort);

            JoinThread(tt);

            // 用这个新的连接接收文件数据
            {
                tPtclMsg RecvMessage;
                FTP_RecvMessage(nClientSocket, &RecvMessage);
                ((char *)RecvMessage.m_pData)[RecvMessage.m_tHeader.m_nBytes] = '\0';
                printf("3");
                printf("filedata: \n%s\n", (char *)RecvMessage.m_pData);
                ASCII_write(filename, (char *)RecvMessage.m_pData);
            }
            printf("9\n");

            // 关闭这个数据传输的连接
            FTP_CloseConnection(nClientSocket);
            // 关闭服务
            FTP_CloseConnection(dSocket);
        }
    }
    if(nRecvMessage.m_tHeader.m_nType==FTP_DOWNLOAD){
        char filename[30];
        memcpy(filename, nRecvMessage.m_pData, nRecvMessage.m_tHeader.m_nBytes);
        printf("filename: %s\n", filename);
        if(tRecvMessage.m_tHeader.m_nType==FTP_PORT){
            // 服务器20端口连接客户端的随机端口
            in_port_t dPort = (in_port_t)atol(tRecvMessage.m_pData);
            struct sockaddr_in client_addr;
            SOCKET dSocket;
            // SOCKET dSocket = FTP_PortConnect(pClientInfo->m_tClientAddress, dPort);
            {
                if((dSocket=make_stream_client_socket())==-1)    // 创建 TCP socket 失败
                    oops("cannot make socket",2);
                bzero(&client_addr,sizeof(client_addr)); // 初始化服务器地址
                client_addr.sin_family = AF_INET;
                client_addr.sin_port = htons(dPort);
                client_addr.sin_addr.s_addr = pClientInfo->m_tClientAddress;
                // inet_pton(AF_INET, server, &server_addr.sin_addr);

                struct sockaddr_in client;
                client.sin_family = AF_INET;
                client.sin_addr.s_addr = pClientInfo->m_tClientAddress;
                client.sin_port = DPORT;
                // 绑定本地20端口
                if(bind(dSocket, (struct sockaddr *)&client, sizeof(struct sockaddr_in))==INVALID_SOCKET){
                    printf("Failed to bind at PerformRoutine.\n");
                    return INVALID_SOCKET;
                }

                if(connect( dSocket, ( struct sockaddr* )&client_addr, sizeof( client_addr ) )==-1)
                    oops("cannot connect", 5);
            }
            if(dSocket==-1){
                printf("Failed to PORT connect client.\n");
                return -1;
            }
            // 用这个新的连接发送文件数据
            {
                tPtclMsg SendMessage;
                SendMessage.m_tHeader.m_nType = FTP_ASCII;
                char buf[1024];
                ASCII_read(filename, buf, &SendMessage.m_tHeader.m_nBytes);
                SendMessage.m_pData = buf;
                if(FTP_SendMessage(dSocket, &SendMessage)==-1){
                    printf("Failed to send file data to client.\n");
                    return -1;
                }
                buf[SendMessage.m_tHeader.m_nBytes] = '\0';
                printf("filedata: %s\n",buf);
            }
            // 关闭这个数据传输的连接
            FTP_CloseConnection(dSocket);
        }
        if(tRecvMessage.m_tHeader.m_nType==FTP_PASV){    
            // 服务器开启随机端口等待客户端连接
            int dPort;
            SOCKET dSocket = FTP_RunDataListener(&dPort, pClientInfo->m_tClientAddress);
            if(dSocket==-1){
                printf("Cannot start data listener.\n");
                return -1;
            }

            printf("dPort: %d\n", dPort);

            // 等待用户连接
            in_addr_t tClientAddress;
            SOCKET nClientSocket;
            PORT_Accept_arg PAarg;
            PAarg.m_dSocket = dSocket;
            PAarg.m_ptServerAddress = &tClientAddress;
            PAarg.m_pnServerSocket = &nClientSocket;
            tThread tt;
            StartThread(&tt, PORT_Accept, (void *)&PAarg);

            // PASV模式下用21端口告知用户连接服务器的一个随机端口号
            tPtclMsg pMsg;
            char sPort[6];
            portToStr(sPort, dPort);
            pMsg.m_tHeader.m_nType = FTP_PASV;
            pMsg.m_tHeader.m_nBytes = sizeof(sPort);
            pMsg.m_pData = sPort;
            FTP_SendMessage(pClientInfo->m_nSocket, &pMsg);
            printf("sPort: %s\n", sPort);

            JoinThread(tt);

            // 用这个新的连接发送文件数据
            {
                tPtclMsg SendMessage;
                SendMessage.m_tHeader.m_nType = FTP_ASCII;
                char buf[1024];
                ASCII_read(filename, buf, &SendMessage.m_tHeader.m_nBytes);
                SendMessage.m_pData = buf;
                if(FTP_SendMessage(nClientSocket, &SendMessage)==-1){
                    printf("Failed to send file data to client.\n");
                    return -1;
                }
                buf[SendMessage.m_tHeader.m_nBytes] = '\0';
                printf("filedata: %s\n",buf);
            }
            printf("9\n");

            // 关闭这个数据传输的连接
            FTP_CloseConnection(nClientSocket);
            // 关闭服务
            FTP_CloseConnection(dSocket);
        }
    }
    return 0;
}

int main(int argc, char **argv){
    signal(SIGTERM, SignalHandler);
    int count = 0;
    InitializeMutex(&g_tCounterMutex);
    if(StartScheduler()==-1)
        return -1;
    TerminateCommand();
    return 0;
}

void SignalHandler(int nSignalNo){
    if(nSignalNo != SIGTERM)
        return;
    TerminateProgram();
}

void TerminateProgram(){
    exit(0);
}

void TerminateCommand(){
    char szCommandBuffer[COMMAND_BUFFER];
    for(;;){
        printf(">");
        fgets(szCommandBuffer, COMMAND_BUFFER, stdin);
        if(strncmp("exit", szCommandBuffer, 4)==0||
        strncmp("quit",szCommandBuffer,4)==0||
        strncmp("bye",szCommandBuffer,3)==0)
            break;
    }
}

int FTP_CloseConnection(SOCKET nSocket){
    close(nSocket);
}

int StartScheduler(){
    StartServer();
}