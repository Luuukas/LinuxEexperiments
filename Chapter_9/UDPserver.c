// UDPserver 8080

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "UDPfns.c"

#define oops(m,x) { perror(m); exit(x); }
#define BUFSIZ 512

int make_dagram_server_socket(int);
int get_internet_address(char *, int, int *, struct sockaddr_in *);
void say_who_called(struct sockaddr_in *);

int main(int ac, char *av[]){
    int port;
    int sock;
    char buf[BUFSIZ];    // 接收数据缓存
    size_t msglen;    // 接收到的数据长度
    struct sockaddr_in saddr;    // 存放发送者地址
    socklen_t saddrlen;
    
    if(ac==1||(port=atoi(av[1]))<=0){
        fprintf(stderr, "usage: %s portnumber\n", av[0]);
        exit(1);
    }

    if((sock=make_dagram_server_socket(port))==-1)    // 建立 UDP 套接字失败
        oops("cannot make socket", 2);
    saddrlen = sizeof(saddr);
    while(recvfrom(sock, &buf, BUFSIZ, 0, (struct sockaddr *)&saddr, &saddrlen)!=-1){
        say_who_called(&saddr);
    }
    return 0;
}

void say_who_called(struct sockaddr_in *addrp){
    char host[BUFSIZ];
    int port;

    get_internet_address(host, BUFSIZ, &port, addrp);
    printf("    from: %s:%d\n", host, port);
}