/***** threadexittest.c *****/
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void *create(void *arg){
    printf("new thread is created ...\n");
    return (void *)2;
}

int main(int argc, char *argv[]){
    pthread_t tid;
    int error;
    void *temp;
    error = pthread_create(&tid, NULL, create, NULL);
    if(error!=0){
        printf("thread is not created ...");
        return -1;
    }
    error = pthread_join(tid, &temp);
    if(error!=0){
        printf("thread is not exit ...");
        return -2;
    }
    printf("thread is exit code %d \n", (int)temp);
    sleep(1);
    printf("thread is created ...");
    return 0;
}