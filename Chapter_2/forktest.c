/***** forktest.c *****/
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main(){
    int i;
    int p_id;
    if((p_id = fork())==0){
        /* 子进程程序 */
        for(i=1;i<3;++i)
            printf("This is child process\n");
    }else if(p_id==-1){
        printf("fork new process error!\n");
        _exit(-1);
    }else{
        /* 父进程程序 */
        for(i=1;i<3;++i)
            printf("This is parent process\n");
    }
    return 0;
}