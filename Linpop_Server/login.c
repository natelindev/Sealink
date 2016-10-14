#include "common.h"

pthread_mutex_t g_LoginMutex;
PCLIENT g_Online;


void __attribute__((constructor)) LoginInit(void) {     // .init_array 
    pthread_mutex_init(&g_LoginMutex, NULL);
    g_Online = NULL;
    return;
}

PCLIENT login_FindOnlineClientByUID(uint32_t uid) {
    PCLIENT result = NULL, p;

    pthread_mutex_lock(&g_LoginMutex);
    p = g_Online;
    while(p) {
        if(p->uid == uid) {
            result = p;
            break;
        }
        p = p->next;
    }
    pthread_mutex_unlock(&g_LoginMutex);
    return result;
}

int login_DisconnectClient(PCLIENT c) {
    PCLIENT p;
    int result = -1;
    LogEvent(LOG_INFO, "Disconnecting %d", c->uid);

    pthread_mutex_lock(&g_LoginMutex);
    p = g_Online;
    if(p == c) {
        LogEvent(LOG_INFO, "UID %d disconnected", c->uid);
        g_Online = c->next;
        if(c->freq)     {
            free(c->freq->filename);
            free(c->freq);
        }
        close(c->msgsock);
        free(c);
        result = 0;
    } else {
        while(p->next) {
            if(p->next == c) {
                LogEvent(LOG_INFO, "UID %d disconnected", c->uid);
                p->next = p->next->next;
                if(c->freq) {
                    free(c->freq->filename);
                    free(c->freq);
                }
                close(c->msgsock);      // free 
                free(c);                // cleanup

                result = 0;
                break;
            }
            p = p->next;
        }
    }
    pthread_mutex_unlock(&g_LoginMutex);
    return result;
}

PCLIENT login_AddClient(int msgsock, uint32_t uid, in_addr_t client_ip, uint16_t client_port, uint8_t* key, uint8_t* iv) {
    PCLIENT result, p;

    if(login_FindOnlineClientByUID(uid)) {
        LogEvent(LOG_ERROR, "UID %d already online", uid);
        return NULL;
    }
    
    result = (PCLIENT)malloc(sizeof(CLIENT));
    if(!result) {
        LogEvent(LOG_ERROR, "malloc() oom?");
        return NULL;
    }

    bzero(result, sizeof(CLIENT));
    result->msgsock = msgsock;
    result->uid = uid;
    result->client_ip = client_ip;
    result->client_port = client_port;
    result->key = key;
    result->iv = iv;

    pthread_mutex_lock(&g_LoginMutex);
    p = g_Online;
    if(!p) {
        g_Online = result;
    } else {
        while(p) {
            if(p->next == NULL) {
                p->next = result;
                break;
            }
            p = p->next;
        }
    }
    pthread_mutex_unlock(&g_LoginMutex);
    return result;
}

PFILE_REQUEST login_GetFileReq(PCLIENT target) {
    PFILE_REQUEST freq = NULL;

    pthread_mutex_lock(&g_LoginMutex);
    freq = target->freq;
    pthread_mutex_unlock(&g_LoginMutex);
    return freq;
}

int login_AddFileReq(PCLIENT target, PFILE_REQUEST freq) {
    int result = 0;
    pthread_mutex_lock(&g_LoginMutex);
    if(target->freq) {
        result = -1;
    } else {
        target->freq = freq;
    }
    pthread_mutex_unlock(&g_LoginMutex);
    return result;
}

void login_RemoveFileReq(PCLIENT target) {
    PFILE_REQUEST freq;
    pthread_mutex_lock(&g_LoginMutex);
    freq = target->freq;
    target->freq = NULL;
    pthread_mutex_unlock(&g_LoginMutex);

    if(freq)
    {
        free(freq->filename);
        free(freq);
    }

    return;
}

void login_NotifyAllLogin(PCLIENT c, uint32_t disconnect) {
    PCLIENT p;
    int w;

    pthread_mutex_lock(&g_LoginMutex);
    p = g_Online;
    while(p) {
        if(c == p) {
            p = p->next;
            continue;
        }
        PUSER_MSG msg = malloc(sizeof(USER_MSG));
        msg->magic = USERLOG_MAGIC;
        msg->uid = -1;  // broadcast
        msg->msglen = sizeof(UMSG_LOGIN);
        w = srv_WritePacket(p->msgsock, msg, sizeof(USER_MSG), p->key, p->iv);
        free(msg);
        if(w >= 0) {
            PUMSG_LOGIN lmsg = malloc(sizeof(UMSG_LOGIN));
            lmsg->disconnect = disconnect;
            lmsg->uid = c->uid;
            lmsg->ipaddr = c->client_ip;
            w = srv_WritePacket(p->msgsock, lmsg, sizeof(UMSG_LOGIN), p->key, p->iv);
            free(lmsg);
        }
        p = p->next;
    }
    pthread_mutex_unlock(&g_LoginMutex);
    return;
}

void login_NotifyAll(PCLIENT c, uint32_t magic, uint8_t* msgpacket, uint32_t size) {
    PCLIENT p;
    int w;

    pthread_mutex_lock(&g_LoginMutex);
    p = g_Online;
    while(p) {
        if(c == p) {
            p = p->next;
            continue;
        }
        PUSER_MSG msg = malloc(sizeof(USER_MSG));
        msg->magic = magic;
        msg->uid = c->uid;  // broadcast
        msg->msglen = size;
        w = srv_WritePacket(p->msgsock, msg, sizeof(USER_MSG), p->key, p->iv);
        free(msg);
        if(w >= 0) {
            w = srv_WritePacket(p->msgsock, msgpacket, size, p->key, p->iv);
        }
        p = p->next;
    }
    pthread_mutex_unlock(&g_LoginMutex);
    return;
}

void login_NotifyAllChangeName(PCLIENT c, char* newname) {

    if(strlen(newname) > 25)    return;
    char* a = strdup(newname);
    login_NotifyAll(c, USERNAME_MAGIC, a, strlen(a));
    free(a);
    return;
}

void login_Notify(uint32_t fuid, uint32_t uid, uint32_t magic, uint8_t* msgpacket, uint32_t size) {
    PCLIENT target;

    target = login_FindOnlineClientByUID(uid);
    if(!target){
        LogEvent(LOG_WARNING, "Trying to notify user (%d) when not online", uid);
        return;
    }
    PUSER_MSG msg = malloc(sizeof(USER_MSG));
    msg->magic = magic;
    msg->uid = fuid;
    msg->msglen = size;

    /* RACE CONDITION WARNING !!! */
    int w = srv_WritePacket(target->msgsock, msg, sizeof(USER_MSG), target->key, target->iv);
    if(w >= 0 && size) {
        w = srv_WritePacket(target->msgsock, msgpacket, size, target->key, target->iv);
    }
    free(msg);

    return;
}