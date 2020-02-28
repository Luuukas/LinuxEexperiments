#include <pthread.h>

typedef pthread_mutex_t tMutex;

void InitializeMutex(tMutex *pMutex);

int DestroyMutex(tMutex *pMutex);

int LockMutex(tMutex *pMutex);

int UnlockMutex(tMutex *pMutex);