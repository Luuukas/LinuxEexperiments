#include "sFTPclient.h"
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

void *PORT_Accept(void *arg){
    PORT_Accept_arg *PAarg = (PORT_Accept_arg*)arg;
    *(PAarg->m_pnServerSocket) = FTP_Accept(PAarg->m_dSocket, PAarg->m_ptServerAddress);
    printf("Done. nServerSocket: %d\n",*(PAarg->m_pnServerSocket));
}

// int make_internet_address(char *hostname, int port, struct sockaddr_in *addrp){
//     // 通过主机名和端口号构建 Internet socket 地址
//     struct hostent *hp;

//     bzero((void*)addrp, sizeof(struct sockaddr_in));
//     hp = gethostbyname(hostname);
//     // printf("%s\n",hostname);
//     if(hp==NULL) return -1;
//     printf("hp->h_length: %d\n", hp->h_length);
//     bcopy((void *)hp->h_addr, (void *)&addrp->sin_addr, hp->h_length);
//     addrp->sin_port = htons(port);
//     addrp->sin_family = AF_INET;
//     return 0;
// }

int main(int argc, char **argv){
    char server[30] = "127.0.0.1", user[20] = "Luuukas", passwd[20] = "Chong516";

    tPtclType modeFlag = FTP_ASCII;    // 传输模式，默认为ASCII模式
    tPtclType workFlag = FTP_PORT;    // 工作模式，默认为PORT模式
    int port = 8080, socket;
    if(!check_format(argv[1], server, user, passwd, &port))    // 不符合ftps格式
        return -1;

    struct sockaddr_in server_addr;

    // {
    //     if((socket=make_stream_client_socket())==-1)    // 创建 TCP socket 失败
    //         oops("cannot make socket",2);

    //     if(make_internet_address("Epsilon", port, &server_addr)==-1)
    //         oops("cannot make addr", 4);

    //     if(connect(socket, (struct sockaddr *)&server_addr, sizeof(struct sockaddr))==-1)    // 建立连接失败
    //         oops("cannot connect", 5);
    // }

    {
        if((socket=make_stream_client_socket())==-1)    // 创建 TCP socket 失败
            oops("cannot make socket",2);
        bzero(&server_addr,sizeof(server_addr)); // 初始化服务器地址
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, server, &server_addr.sin_addr);
        if(connect( socket, ( struct sockaddr* )&server_addr, sizeof( server_addr ) )==-1)
            oops("cannot connect", 5);
    }

    char userStr[40];
    memcpy(userStr, user, sizeof(user));
    memcpy(userStr+20, passwd, sizeof(passwd));
    tPtclMsg pSendMsg, pRecvMsg;
    int pRecvCode = -1;
    pSendMsg.m_tHeader.m_nType = FTP_USERAUTH;
    pSendMsg.m_tHeader.m_nBytes = 40;
    pSendMsg.m_pData = userStr;
    FTP_SendMessage(socket, &pSendMsg);    // 发送用户信息给服务器验证
    FTP_RecvCode(socket, &pRecvCode);    // 获取服务器对用户信息的验证结果
    // if(pRecvCode!=200){
    //     printf("Invalid username or password!\n");
    //     exit(1);
    // }
    // 预留一个随机端口用于PORT模式等待服务器连接
    int dPort;
    int dSocket = FTP_RunDataListener(&dPort, server_addr.sin_addr.s_addr);
    printf("dPort: %d\n", dPort);
    // 等待服务器连接
    // 向FTP服务器21端口发送工作模式
    if(workFlag==FTP_PORT){
        char sPort[6];
        portToStr(sPort, dPort);
        pSendMsg.m_tHeader.m_nType = FTP_PORT;
        pSendMsg.m_tHeader.m_nBytes = sizeof(sPort);
        pSendMsg.m_pData = sPort;
        printf("sPort: %s\n",sPort);
        FTP_SendMessage(socket, &pSendMsg);
    }
    if(workFlag==FTP_PASV){
        printf("5\n");
        pSendMsg.m_tHeader.m_nType = FTP_PASV;
        pSendMsg.m_tHeader.m_nBytes = 0;
        FTP_SendMessage(socket, &pSendMsg);
        printf("6\n");
    }

    char comStr[40], command[10], para[30];

    while(1){
        printf("ftp> ");
        fgets(comStr, 40, stdin);
        printf("echo: %s",comStr);
        /*分析comStr，获取command及para部分*/
        int tot=0;
        while(comStr[tot]==' '){
            ++tot;
        }
        int i=0;
        for(;i<10-1;i++,tot++){
            if(comStr[tot]==' '||comStr[tot]=='\n') break;
            command[i] = comStr[tot];
            printf("%d",i);
        }
        command[i] = '\0';
        if(comStr[tot]!=' '&&comStr[tot]!='\n'){
            printf("Command is too long, no such command.\n");
            continue;
        }
        while(comStr[tot]==' '){
            ++tot;
        }
        int j=0;
        while(j<30-1&&comStr[tot]!='\n'){
            para[j] = comStr[tot];
            ++j;++tot;
        }
        para[j] = '\0';
        if(comStr[tot]!='\n'){
            printf("Para is too long.\n");
            continue;
        }

        printf("command: %s\n", command);
        printf("para: %s\n",para);

        if(strcmp(command, "quit")==0){
            break;
        }
        if(strcmp(command, "get")==0){
            char filename[30] = "chen.txt";
            SOCKET nServerSocket = -10086;
            pSendMsg.m_tHeader.m_nType = FTP_DOWNLOAD;
            pSendMsg.m_tHeader.m_nBytes = sizeof(para);
            pSendMsg.m_pData = para;
            if(workFlag==FTP_PORT){
                in_addr_t tServerAddress;
                PORT_Accept_arg PAarg;
                PAarg.m_dSocket = dSocket;
                PAarg.m_ptServerAddress = &tServerAddress;
                PAarg.m_pnServerSocket = &nServerSocket;
                tThread tt;
                // 开启线程等待连接
                StartThread(&tt, PORT_Accept, (void *)&PAarg);
                // nServerSocket = FTP_Accept(dSocket, &tServerAddress);
                printf("0");
                // 向FTP 21端口发送下载文件的请求
                FTP_SendMessage(socket, &pSendMsg);
                // 连接成功后直接接收服务器传来的文件数据
                printf("1");
                JoinThread(tt);
                FTP_RecvMessage(nServerSocket, &pRecvMsg);
                FTP_CloseConnection(nServerSocket);
                FTP_CloseConnection(dSocket);
            }
            if(workFlag==FTP_PASV){
                // 向FTP 21端口发送下载文件的请求
                FTP_SendMessage(socket, &pSendMsg);
                printf("7\n");
                // 接收服务器21端口传来的随机端口号
                FTP_RecvMessage(socket, &pRecvMsg);
                printf("8\n");
                // 取出端口号
                in_port_t dPort = (in_port_t)atol(pRecvMsg.m_pData);
                printf("dPort: %d\n", dPort);
                struct sockaddr_in d_server_addr;
                // 建立到这个端口的连接
                {
                    if((dSocket=make_stream_client_socket())==-1)    // 创建 TCP socket 失败
                        oops("cannot make socket",2);
                    bzero(&d_server_addr,sizeof(d_server_addr)); // 初始化服务器地址
                    d_server_addr.sin_family = AF_INET;
                    d_server_addr.sin_port = htons(dPort);
                    inet_pton(AF_INET, server, &d_server_addr.sin_addr);
                    // inet_pton(AF_INET, server, &server_addr.sin_addr);
                    if(connect( dSocket, ( struct sockaddr* )&d_server_addr, sizeof( d_server_addr ) )==-1)
                        oops("cannot connect", 5);
                }
                FTP_RecvMessage(dSocket, &pRecvMsg);
                printf("9\n");
                // 关闭这个数据传输的连接
                FTP_CloseConnection(dSocket);
            }
            // 写入文件
            printf("2");
            if(modeFlag==FTP_ASCII){
                ((char *)pRecvMsg.m_pData)[pRecvMsg.m_tHeader.m_nBytes] = '\0';
                printf("3");
                printf("filedata: \n%s\n", (char *)pRecvMsg.m_pData);
                ASCII_write(filename, (char *)pRecvMsg.m_pData);
            }
            if(modeFlag==FTP_BIN){
                BIN_write(filename, (char *)pRecvMsg.m_pData);
            }
        }
        if(strcmp(command, "put")==0){
            char filename[30] = "chong.txt";
            SOCKET nServerSocket = -10086;
            pSendMsg.m_tHeader.m_nType = FTP_UPLOAD;
            pSendMsg.m_tHeader.m_nBytes = sizeof(para);
            pSendMsg.m_pData = para;
            char buf[1024];
            int bytes;
            if(modeFlag==FTP_ASCII){
                ASCII_read(filename, buf, &bytes);
                buf[bytes] = '\0';
                printf("filedata: %s\n",buf);
            }
            if(modeFlag==FTP_BIN){
                BIN_read(filename, buf, &bytes);
            }
            if(workFlag==FTP_PORT){
                in_addr_t tServerAddress;
                PORT_Accept_arg PAarg;
                PAarg.m_dSocket = dSocket;
                PAarg.m_ptServerAddress = &tServerAddress;
                PAarg.m_pnServerSocket = &nServerSocket;
                tThread tt;
                // 开启线程等待连接
                StartThread(&tt, PORT_Accept, (void *)&PAarg);
                // nServerSocket = FTP_Accept(dSocket, &tServerAddress);
                printf("0");
                // 向FTP 21端口发送上传文件的请求
                FTP_SendMessage(socket, &pSendMsg);
                // 连接成功后直接向服务器发送文件数据
                printf("1");
                JoinThread(tt);

                pSendMsg.m_tHeader.m_nType = modeFlag;
                pSendMsg.m_tHeader.m_nBytes = bytes;
                pSendMsg.m_pData = buf;

                FTP_SendMessage(nServerSocket, &pSendMsg);
                FTP_CloseConnection(nServerSocket);
                FTP_CloseConnection(dSocket);
            }
            if(workFlag==FTP_PASV){
                // 向FTP 21端口发送下载文件的请求
                FTP_SendMessage(socket, &pSendMsg);
                printf("7\n");
                // 接收服务器21端口传来的随机端口号
                FTP_RecvMessage(socket, &pRecvMsg);
                printf("8\n");
                // 取出端口号
                in_port_t dPort = (in_port_t)atol(pRecvMsg.m_pData);
                printf("dPort: %d\n", dPort);
                struct sockaddr_in d_server_addr;
                // 建立到这个端口的连接
                {
                    if((dSocket=make_stream_client_socket())==-1)    // 创建 TCP socket 失败
                        oops("cannot make socket",2);
                    bzero(&d_server_addr,sizeof(d_server_addr)); // 初始化服务器地址
                    d_server_addr.sin_family = AF_INET;
                    d_server_addr.sin_port = htons(dPort);
                    inet_pton(AF_INET, server, &d_server_addr.sin_addr);
                    // inet_pton(AF_INET, server, &server_addr.sin_addr);
                    if(connect( dSocket, ( struct sockaddr* )&d_server_addr, sizeof( d_server_addr ) )==-1)
                        oops("cannot connect", 5);
                }
                pSendMsg.m_tHeader.m_nType = modeFlag;
                pSendMsg.m_tHeader.m_nBytes = bytes;
                pSendMsg.m_pData = buf;

                FTP_SendMessage(dSocket, &pSendMsg);
                printf("9\n");
                // 关闭这个数据传输的连接
                FTP_CloseConnection(dSocket);
            }
        }
    }
}

int check_format(char *command, char *server, char *user, char *passwd, int *port){
    // server = "127.0.0.1";
    // user = "Luuukas";
    // passwd = "Chong516";
    // *port = 21;
    return 1;
}

SOCKET FTP_RunDataListener(int* nPort, in_addr_t serverInAddr){
    SOCKET nServerSocket = INVALID_SOCKET;
    struct sockaddr_in tAddress;
    nServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(nServerSocket==INVALID_SOCKET){
        return INVALID_SOCKET;
    }
    tAddress.sin_port = htons((unsigned short)8081);    // 系统随机分配端口
    tAddress.sin_family = AF_INET;
    tAddress.sin_addr.s_addr = htonl(INADDR_ANY);    // 只允许FTP服务器连接
    if(bind(nServerSocket, (const struct sockaddr *)&tAddress, sizeof(struct sockaddr_in))==INVALID_SOCKET){
        return -1;
    }
    if(listen(nServerSocket, SOMAXCONN)==INVALID_SOCKET){
        return INVALID_SOCKET;
    }
    *nPort =  ntohs(tAddress.sin_port);    // 保存系统分配的端口号
    return nServerSocket;
}

SOCKET FTP_Accept(SOCKET nSocket, in_addr_t *ptClientIP){
    struct sockaddr_in tAddr_in;
    socklen_t nLength = sizeof(struct sockaddr_in);
    SOCKET nAccSocket = accept(nSocket, (struct sockaddr*)&tAddr_in, &nLength);
    printf("nAccSocket: %d\n", nAccSocket);

    if(nAccSocket<0){
        return INVALID_SOCKET;
    }

    if(ptClientIP){
        *ptClientIP = tAddr_in.sin_addr.s_addr;
    }
    return nAccSocket;
}

int FTP_CloseConnection(SOCKET nSocket){
    close(nSocket);
}