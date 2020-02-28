/***** alarmtest.c *****/
#include <signal.h>
#include <unistd.h>
static void sig_alrm(int signo){
    return;
}

unsigned int mysleep(unsigned int nsecs){
    if(signal(SIGALRM, sig_alrm)==SIG_ERR)
        return (nsecs);
    alarm(nsecs);    // 设置定时器
    pause();    // 等待信号
    return(alarm(0));
}

int main(){
    printf("sleep\n");
    mysleep(10);
    printf("wake up\n");
}