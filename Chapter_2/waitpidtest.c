/***** waitpidtest.c *****/
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
void main(){
    pid_t pc, pr;
    pc = fork();
    if(pc<0) printf("Error occured on forking\n");
    else if(pc==0){    // 如果是子进程
        sleep(10);
        exit(0);
    }
    // 如果是父进程
    do{
        pr=waitpid(pc, NULL, WNOHANG);    // 使用WNOHANG参数，waitpid不会在这里等待
        if(pr==0){
            printf("No child exited\n");
            sleep(1);
        }
    }while(pr==0);
    if(pr==pc) printf("successfully get child %d\n",pr);
    else printf("some error occured\n");
}