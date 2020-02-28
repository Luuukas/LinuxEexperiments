#include "sFTPfns.h"
const int32_t UMPP_END_MARKER = __INT32_MAX__;

int FTP_Send(SOCKET nSocket, const void *pBuffer, int nSize){
    return (int)send(nSocket, pBuffer, nSize, 0);
}

int FTP_SendUntilAll(SOCKET nSocket, const void *pBuffer, int nSize){
    int nSent = 0;
    int nSentBytes;
    for(;;){
        nSentBytes = FTP_Send(nSocket,(void*)&((char*)pBuffer)[nSent],nSize-nSent);
        if(nSentBytes<=0)
            return -1;
        nSent += nSentBytes;
        if(nSent==nSize)
            return 0;
    }
}

int FTP_Response(SOCKET nSocket, int code){
    char bufCode[4] = {
        (char)(code/100),
        (char)(code/10%10),
        (char)(code%10),
        '\0'
    };
    if(FTP_SendUntilAll(nSocket, bufCode, sizeof(char)*4)==-1){
        printf("Connection lost at responsing code: FTP_SResponse\n");
        return -1;
    }
    return 0;
}

int FTP_SendHeader(SOCKET nSocket, const tPtclHeader *pHeader){
    uint32_t bufHeader[4];
    bufHeader[0] = htonl(pHeader->m_nVersion);
    bufHeader[1] = htonl(pHeader->m_nType);
    bufHeader[3] = htonl(pHeader->m_nBytes);
    if(FTP_SendUntilAll(nSocket, bufHeader, sizeof(uint32_t)*4)==-1){
        printf("Connection lost at sending packet header: FTP_SendMessage\n");
        return -1;
    }
    return 0;
}

int FTP_SendMessage(SOCKET nSocket, const tPtclMsg *pMsg){
    uint32_t bufEndMarker = htonl(UMPP_END_MARKER);
    if(!pMsg||nSocket==-1||(pMsg->m_tHeader.m_nBytes!=0&&!pMsg->m_pData)){
        return -1;
    }
    if(FTP_SendHeader(nSocket, &pMsg->m_tHeader)==-1)
        return -1;
    if(pMsg->m_tHeader.m_nBytes!=0){
        if(FTP_SendUntilAll(nSocket, pMsg->m_pData, pMsg->m_tHeader.m_nBytes)==-1){
            printf("Connection lost at sending packet message: FTP_SendMessage(%d)\n", nSocket);
            return -1;
        }
    }
    if(FTP_SendUntilAll(nSocket, &bufEndMarker, sizeof(uint32_t))==-1){
        printf("Connection lost at sending packet message: FTP_SendMessage(%d)\n", nSocket);
        return -1;
    }
}

int FTP_Receive(SOCKET nSocket, void *pBuffer, int nSize){
    return (int)recv(nSocket, pBuffer, nSize, 0);
}

static int ReceiveUntilFull(SOCKET nSocket, void *pBuffer, int nSize){
    int nReceived = 0;
    int nGotBytes;
    for(;;){
        nGotBytes = FTP_Receive(nSocket,(void*)&((char*)pBuffer)[nReceived],nSize-nReceived);
        if(nGotBytes<=0)
            return -1;
        nReceived += nGotBytes;
        if(nReceived==nSize)
            return 0;
    }
}

int FTP_RecvCode(SOCKET nSocket, int* code){
    char bufCode[4];
    if(!code||nSocket==-1){
        return -1;
    }
    if(ReceiveUntilFull(nSocket, bufCode, 4*sizeof(char))==-1){
        printf("Connection lost at receiving code: FTP_RecvCode(%d)\n",nSocket);
        return -1;
    }
    *code = atoi(bufCode);
    return 0;
}

int FTP_RecvMessage(SOCKET nSocket, tPtclMsg *pMsg){
    uint32_t bufHeader[4];
    uint32_t bufEndMarker;
    if(!pMsg||nSocket==-1){
        return -1;
    }
    pMsg->m_pData = NULL;
    if(ReceiveUntilFull(nSocket, bufHeader, 4*sizeof(uint32_t))==-1){
        printf("Connection lost at receiving packet header: FTP_RecvMessage(%d)\n",nSocket);
        return -1;
    }
    pMsg->m_tHeader.m_nVersion=ntohl(bufHeader[0]);
    pMsg->m_tHeader.m_nType = ntohl(bufHeader[1]);
    pMsg->m_tHeader.m_nBytes = ntohl(bufHeader[3]);

    if(pMsg->m_tHeader.m_nBytes==0){
        pMsg->m_pData = NULL;
    }else{
        pMsg->m_pData = malloc(pMsg->m_tHeader.m_nBytes);
        if(!pMsg->m_pData){
            printf("Cannot malloc %d bytes for getting data at FTP_RecvMessage(%d)\n",pMsg->m_tHeader.m_nBytes, nSocket);
            return -1;
        }
        if(ReceiveUntilFull(nSocket, pMsg->m_pData, pMsg->m_tHeader.m_nBytes)==-1){
            printf("Connection lost at receiving message: FTP_RecvMessage(%d)\n", nSocket);
            SAFE_RELEASE(pMsg->m_pData)
            return -1;
        }
    }
    if(ReceiveUntilFull(nSocket,&bufEndMarker, sizeof(uint32_t))==-1){
        printf("Connection lost at receiving end marker: FTP_RecvMessage(%d)\n", nSocket);
        SAFE_RELEASE(pMsg->m_pData);
        return -1;
    }
    bufEndMarker = ntohl(bufEndMarker);
    if(bufEndMarker!=UMPP_END_MARKER){
        printf("Recieved non-EndMarker %x: FTP_RecvMessage(%d)\n", bufEndMarker, nSocket);
        return -1;
    }
    return 0;
}