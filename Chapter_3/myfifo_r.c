#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
// 为了open，close等操作
#include <fcntl.h>
#define FIFO_SERVER "./mytest.fifo"
int main()
{
    umask(0);
    if(mkfifo(FIFO_SERVER, 0664)<0){
        if(errno==EEXIST){
            // 文件已存在无需再次创建
        }else{
            perror("mkfifo error");
            return -1;
        }
    }
    int fd = open(FIFO_SERVER, O_RDONLY);
    if(fd<0){
        perror("open fifo error");
        return -1;
    }
    printf("%s is open, now you can read\n");
    while(1){
        char buff[1024] = {0};
        int ret = read(fd, buff, 1023);
        // ret == 0代表没有读到东西
        if(ret>0){
            printf("read:[%s]\n", buff);
        }else if(ret==0){
            printf("all write is close!\n");
            sleep(1);
        }
    }
    close(fd);
    return 0;
}