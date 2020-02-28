/*
    UDP 编程的客户端一般步骤是：
    1. 使用函数 socket()，创建一个套接字。
    2. 使用函数 setsockopt()，设置套接字属性（可选）。
    3. 使用函数 bind()，将IP地址、端口等信息绑定到套接字上（可选）。
    4. 设置对方的IP地址和端口等属性。
    5. 使用函数 sendto() 发送数据。
    6. 关闭网络连接。
*/
// UDPclient Epsilon 8080 'message'
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "UDPfns.c"

#define oops(m,x) { perror(m); exit(x); }

int make_dgram_client_socket();
int make_internet_address(char *, int, struct sockaddr_in *);

int main(int ac, char *av[]){
    if(ac!=4){
        fprintf(stderr, "usage: %s host port 'message'\n", av[0]);
        exit(1);
    }
    char *msg = av[3];
    int sock_id;
    if((sock_id=make_dgram_client_socket())==-1)    // 创建 UDP socket 失败
        oops("cannot make socket", 2);
    struct sockaddr_in to;
    if(make_internet_address(av[1],atoi(av[2]),&to))    // IP 地址转换失败
        oops("cannot make addr", 4);
    if(sendto(sock_id,msg,strlen(msg),0,(struct sockaddr *)&to,sizeof(struct sockaddr))==-1)    // 发送数据失败
        oops("sendto failed",3);
    return 0;
}