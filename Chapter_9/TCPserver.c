// TCPserver 8080

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>

#include "TCPfns.c"

int make_stream_server_socket(int);
int get_internet_address(char *, int, int *, struct sockaddr_in *);

#define oops(m,x) { perror(m); exit(x); }
#define BUFFSIZ 512

int main(int argc, char **argv){
    int port;
    if(argc!=2||(port=atoi(argv[1]))<=0){
        fprintf(stderr, "usage: %s portnumber\n", argv[0]);
        exit(1);
    }
    int sock;
    if((sock=make_stream_server_socket(port))==-1)    // 建立 TCP 套接字失败
        oops("cannot make socket", 2);
    int backlog = 1;
    if(listen(sock, 1)==-1)
        oops("listen() error", 3);
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    int connectfd;
    char recvData[200];    // 接收数据缓存
    int ret;
    while(1){
        printf("waiting for connect\n");
        if((connectfd=accept(sock, (struct sockaddr *)&from, &fromlen))==-1)
            oops("accept() error", 4);

        printf("接收到一个连接：%s\r\n", inet_ntoa(from.sin_addr));
        while(1){
            printf("读取消息：");
            recvData[0] = '\0';
            ret = recv(connectfd, recvData, 200, 0);
            if (ret < 0)
            {
                printf("something wrong\n");
                continue;
            }
            recvData[ret] = '\0';
            if (strcmp(recvData, "quit") == 0)
                break;
            printf("%s\n", recvData);
        }
    }
    return 0;
}

/*
这里只描述同步Socket的recv函数的执行流程。当应用程序调用recv函数时，recv先等待s的发送缓冲中的数据被协议传送完毕，
如果协议在传送s的发送缓冲中的数据时出现网络错误，那么recv函数返回SOCKET_ERROR，如果s的发送缓冲中没有数据或者数据被协议成功发送完毕后，
recv先检查套接字s的接收缓冲区，如果s接收缓冲区中没有数据或者协议正在接收数据，那么recv就一直等待，只到协议把数据接收完毕。
当协议把数据接收完毕，recv函数就把s的接收缓冲中的数据copy到buf中（注意协议接收到的数据可能大于buf的长度，
所以在这种情况下要调用几次recv函数才能把s的接收缓冲中的数据copy完。recv函数仅仅是copy数据，真正的接收数据是协议来完成的），
recv函数返回其实际copy的字节数。如果recv在copy时出错，那么它返回SOCKET_ERROR；如果recv函数在等待协议接收数据时网络中断了，那么它返回0。
*/