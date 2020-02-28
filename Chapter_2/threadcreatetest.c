/***** threadcreatetest.c *****/
// gcc-7 threadcreatetest.c -o threadcreatetest -lpthread
#include <pthread.h>
#include <stdio.h>
void *create(void *arg){
    printf("new thread created ...");
    return NULL;
}

int main(int argc, char *argv[]){
    pthread_t tidp;
    int error;
    error=pthread_create(&tidp,NULL,create,NULL);
    if(error!=0){
        printf("pthread_create is not created ...");
        return -1;
    }
    printf("pthread_create is created ...");
    return 0;
}