/***** alarmtest.c *****/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
void my_alarm_handle(int sign_no)
{
    if(sign_no==SIGALRM){
        printf("I have been waken up.\n");
    }
}

int main()
{
    printf("sleep for 5s ... ...\n");
    signal(SIGALRM, my_alarm_handle);
    alarm(5);
    pause();
    return 0;
}