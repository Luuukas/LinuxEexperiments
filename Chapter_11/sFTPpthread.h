#include <pthread.h>

typedef pthread_t tThread;
typedef void*(*ThreadFunc)(void*);

int StartThread(tThread *pThread, ThreadFunc pFunc, void *pParam);

int JoinThread(tThread hThread);

int DetachThread(tThread hThread);