#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#define HOSTLEN 256
#define h_addr h_addr_list[0]   //保存的是IP地址

int make_internet_address();

int make_dagram_server_socket(int portnum){
    struct sockaddr_in saddr;    // 构建地址信息
    char hostname[HOSTLEN];    // 主机名
    int sock_id;    // 套接字号

    sock_id = socket(PF_INET, SOCK_DGRAM, 0);    // 获取套接字
    if(sock_id==-1) return -1;

    gethostname(hostname, HOSTLEN);    // 获取主机名
    // printf("%s\n",hostname);
    make_internet_address(hostname, portnum, &saddr);

    if(bind(sock_id, (struct sockaddr*)&saddr, sizeof(saddr))!=0) return -1;
    return sock_id;
}

int make_dgram_client_socket(){
    return socket(PF_INET, SOCK_DGRAM, 0);
}

int make_internet_address(char *hostname, int port, struct sockaddr_in *addrp){
    // 通过主机名和端口号构建 Internet socket 地址
    struct hostent *hp;

    bzero((void*)addrp, sizeof(struct sockaddr_in));
    hp = gethostbyname(hostname);
    // printf("%s\n",hostname);
    if(hp==NULL) return -1;
    bcopy((void *)hp->h_addr, (void *)&addrp->sin_addr, hp->h_length);
    addrp->sin_port = htons(port);
    addrp->sin_family = AF_INET;
    return 0;
}

int get_internet_address(char *host, int len, int *portp, struct sockaddr_in *addrp){
    // 从套接字地址中解析主机名和端口号
    strncpy(host, inet_ntoa(addrp->sin_addr), len);
    *portp = ntohs(addrp->sin_port);
    return 0;
}