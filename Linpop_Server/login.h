#ifndef _COMMON_H
#error "Do NOT include this header solely."
#endif

typedef struct _FILE_REQUEST {
    uint32_t from_uid;
    uint32_t filesz;
    char* filename;
} FILE_REQUEST, *PFILE_REQUEST;

typedef struct _CLIENT {
    struct _CLIENT* next;
    int msgsock;
    uint32_t uid;
    in_addr_t client_ip;
    uint16_t client_port;   // host
    uint8_t* key;
    uint8_t* iv;
    PFILE_REQUEST freq;
} CLIENT, *PCLIENT;

PCLIENT login_AddClient(int msgsock, uint32_t uid, in_addr_t client_ip, uint16_t client_port, uint8_t* key, uint8_t* iv);
PCLIENT login_FindOnlineClientByUID(uint32_t uid);
int login_DisconnectClient(PCLIENT c);

PFILE_REQUEST login_GetFileReq(PCLIENT target);
int login_AddFileReq(PCLIENT target, PFILE_REQUEST freq);
void login_RemoveFileReq(PCLIENT target);

void login_NotifyAllLogin(PCLIENT c, uint32_t disconnect);
void login_NotifyAll(PCLIENT c, uint32_t magic, uint8_t* msgpacket, uint32_t size);

void login_NotifyAllChangeName(PCLIENT c, char* newname);
void login_Notify(uint32_t fuid, uint32_t uid, uint32_t magic, uint8_t* msgpacket, uint32_t size);