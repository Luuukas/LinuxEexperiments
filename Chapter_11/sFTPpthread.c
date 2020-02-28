#include "sFTPpthread.h"
int StartThread(tThread *pThread, ThreadFunc pFunc, void *pParam){
    if(pthread_create(pThread, NULL, (void *)pFunc, pParam)==0)
        return 0;
    else
        return -1;
}

int JoinThread(tThread hThread){
    if(pthread_join(hThread, NULL)!=0)
        return -1;
    else
        return 0;
}

int DetachThread(tThread hThread){
    if(pthread_detach(hThread)!=0)
        return -1;
    else
        return 0;
}