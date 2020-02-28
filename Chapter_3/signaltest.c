/***** signaltest.c *****/
/*
    Ctrl+C
    Ctrl+\
    kill -HUB <pid>
    kill -9 <pid>
*/
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
void sigroutine(int dunno)
{    /* 信号处理函数，其中dunno将会得到信号的值 */
    switch (dunno)
    {
    case 1:
        printf("Get a signal -SIGHUP\n");
        break;
    case 2:
        printf("Get a signal -SIGINT\n");
        break;
    case 3:
        printf("Get a signal -SIGQUIT\n");
        break;
    }
}

int main()
{
    printf("process id is %d\n",getpid());
    /* 下面设置3个信号处理函数 */
    signal(SIGHUP, sigroutine);
    signal(SIGINT, sigroutine);
    signal(SIGQUIT, sigroutine);
    for (;;);
}