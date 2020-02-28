/***** execlptest.c *****/
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
char command[256];

void main(){
    int rtn;
    while(1){
        printf(">");
        fgets(command,256,stdin);
        command[strlen(command)-1] = 0;
        if(fork()==0){
            execlp(command, command);
            // 如果exec()返回，表明没有正常执行命令，打印错误信息
            perror(command);
            exit(-1);
        }else{
            wait(&rtn);
            printf("child process return %d\n", rtn);
        }
    }
}