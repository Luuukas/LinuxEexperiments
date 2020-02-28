#include "sFTPcnt.h"

void InitializeMutex(tMutex *pMutex){
    if(pMutex){
        pthread_mutex_init(pMutex, NULL);
    }
}

int DestroyMutex(tMutex *pMutex){
    pthread_mutex_destroy(pMutex);
    return 0;
}

int LockMutex(tMutex *pMutex){
    if(pthread_mutex_lock(pMutex)!=0)
        return -1;
    else
        return 0;
}

int UnlockMutex(tMutex *pMutex){
    if(pthread_mutex_unlock(pMutex)!=0)
        return -1;
    else
        return 0;
}