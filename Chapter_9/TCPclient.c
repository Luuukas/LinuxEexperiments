// TCPclient Epsilon 8080

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>

#include "TCPfns.c"

#define oops(m,x) { perror(m); exit(x); }

int make_stream_client_socket();
int make_internet_address(char *, int, struct sockaddr_in *);

int main(int argc, char **argv){
    if(argc!=3){
        fprintf(stderr, "usage: %s host port\n", argv[0]);
        exit(1);
    }
    int fd;
    int len, ret;
    struct sockaddr_in remoteaddr;
    char data[1024];
    
    int sock_id;
    if((sock_id=make_stream_client_socket())==-1)    // 创建 TCP socket 失败
        oops("cannot make socket",2);
    struct sockaddr_in to;
    if(make_internet_address(argv[1],atoi(argv[2]),&to))    // IP 地址转换失败
        oops("cannot make addr", 4);
    if(connect(sock_id, (struct sockaddr *)&to, sizeof(struct sockaddr))==-1)    // 建立连接失败
        oops("cannot connect", 5);
    while(1){
        scanf("%s",data);
        if(send(sock_id, data, strlen(data), 0)==-1)    // 发送失败
            oops("send fail", 3);
    }
    return 0;
}