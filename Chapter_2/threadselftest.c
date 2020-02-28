/***** threadselftest.c *****/
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
void *create(void *arg){
    printf("new thread ...\n");
    printf("thread_tid==%u \n",(unsigned int)pthread_self());
    printf("thread pid is %d \n",getpid());
    return (void *)0;
}

int main(int argc, char *argv[]){
    pthread_t tid;
    int error;
    printf("Main thread is starting ...\n");
    error=pthread_create(&tid, NULL, create, NULL);
    if(error!=0){
        printf("thread is not created ...\n");
        return -1;
    }
    printf("main pid is %d\n", getpid());
    sleep(1);
    return 0;
}