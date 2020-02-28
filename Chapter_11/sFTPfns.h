#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>

typedef int SOCKET;

typedef enum _tMsgType{
    FTP_USERAUTH,
    FTP_DOWNLOAD,
    FTP_UPLOAD,
    FTP_CD,
    FTP_MKDIR,
    FTP_RMDIR,
    FTP_LS,
    FTP_ASCII,
    FTP_BIN,
    FTP_PORT,
    FTP_PASV,
} tPtclType;

typedef struct _tPtclHeader{
    uint32_t m_nVersion;
    tPtclType m_nType;
    uint32_t m_nBytes;
}tPtclHeader;

typedef struct _tPtclMsg{
    tPtclHeader m_tHeader;
    void *m_pData;
}tPtclMsg;

typedef struct tClientInfo{
    SOCKET m_nSocket;
    in_addr_t m_tClientAddress;
} tClientInfo;

#define SAFE_RELEASE(buf) { free(buf); }

int FTP_Send(SOCKET nSocket, const void* pBuffer, int nSize);

int FTP_SendUntilAll(SOCKET nSocket, const void *pBuffer, int nSize);

int FTP_SendHeader(SOCKET nSocket, const tPtclHeader *pHeader);

int FTP_SendMessage(SOCKET nSocket, const tPtclMsg *pMsg);

int FTP_Receive(SOCKET nSocket, void *pBuffer, int nSize);

static int ReceiveUntilFull(SOCKET nSocket, void *pBuffer, int nSize);

int FTP_RecvMessage(SOCKET nSocket, tPtclMsg* pMsg);

void performRoutine(tPtclMsg tRecvMessage, tClientInfo *pClientInfo);

int FTP_Response(SOCKET nSocket, int code);

int FTP_RecvCode(SOCKET nSocket, int* code);