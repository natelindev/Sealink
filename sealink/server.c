#include "common.h"
#include <openssl/bio.h>
#include <openssl/dh.h>
#include <openssl/pem.h>

/*****************************
*
* TCP/UDP support
*
/*****************************/

uint8_t g_IV[16] = {0};

/*
 * srv_StartListen - creates a tcp socket and start listening
 * returns the newly created socket
 * N.B. - 
 */
int srv_StartListen(uint16_t port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if( sockfd < 0 ) {
        LogEvent(LOG_ERROR, "socket() failed : %s", strerror(errno));
        return -1;
    }
    int reuse = 1;
    if( setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0 ) {
        LogEvent(LOG_WARNING, "setsockopt() failed : %s", strerror(errno));
    }
    struct sockaddr_in sin;
    bzero(&sin,sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    if( bind(sockfd, (struct sockaddr*)&sin, sizeof(sin)) < 0 ) {
        LogEvent(LOG_ERROR, "bind() failed : %s", strerror(errno));
        close(sockfd);
        return -1;
    }
    if( listen(sockfd, 5) < 0 ) {   // maximum backlog 5 or more
        LogEvent(LOG_ERROR, "listen() failed : %s", strerror(errno));
        close(sockfd);
        return -1;
    }
    return sockfd;
}

/* Init signal handler before calling this! */
void __attribute__((noreturn)) srv_ServeForever(int sockfd, srv_handler handler) {  // this should be called by a seperate thread
    struct sockaddr_in incoming;
    uint32_t inaddrlen;
    pthread_t thread;
    while(1) {
        int clisock = accept(sockfd, (struct sockaddr*)&incoming, &inaddrlen);
        if( clisock < 0 ) {
            LogEvent(LOG_ERROR, "accept() failed : %s", strerror(errno));
            pthread_exit((void*)-1);
        }
        LogEvent(LOG_INFO, "Client (%s:%d) connected", inet_ntoa(incoming.sin_addr), ntohs(incoming.sin_port));
        pthread_create(&thread,NULL,handler,(void*)clisock);
    }
}

int srv_StartUDPBinding(uint16_t port) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0) {
        LogEvent(LOG_ERROR, "socket() failed : %s", strerror(errno));
        return -1;
    }
    struct sockaddr_in sin;
    bzero(&sin, sizeof(sin));
    sin.sin_port = htons(port);
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(sockfd, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
        LogEvent(LOG_ERROR, "bind() failed : %s", strerror(errno));
        close(sockfd);
        return -1;
    }
    return sockfd;
}

/* this should be called within a thread */
void __attribute__((noreturn)) srv_ServeUDPForever(int sockfd, srv_udp_handler handler) {
    struct sockaddr_in clin;
    uint32_t clinlen;
    int result = -1;
    uint8_t recvbuf[BUF_SIZE];
    while(1) {
        result = recvfrom(sockfd, recvbuf, BUF_SIZE - 1, 0, (struct sockaddr*)&clin, &clinlen);
        if(result < 0) {
            LogEvent(LOG_ERROR, "recvfrom() failed : %s", strerror(errno));
            exit(-1);
        }
        result = handler(sockfd, recvbuf, result, &clin);
        /* Do exception handling here? */
    }
}

/*****************************
*
* SSL crypto support
*
/*****************************/

static DH* srv_LoadDHParam(char* dhparam_path) {
    BIO* bio;
    DH* result = NULL;
    int codes = 0;
    bio = BIO_new_file(dhparam_path, "rb");
    if(!bio) {
        LogEvent(LOG_ERROR, "BIO_new_file() failed");
        goto out;
    }
    result = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
    if(!result) {
        LogEvent(LOG_ERROR, "PEM_bio_read_DHparams() failed");
        goto bio_out;
    }
    if(!DH_check(result, &codes)) {
        LogEvent(LOG_ERROR, "DH_check() failed");
        DH_free(result);
        result = NULL;
    }
bio_out:
    BIO_free(bio);
out:
    return result;
}

/*
 * srv_DoHandshake - do diffie-hellman key agreement
 * returns the agreed key, g_IV is set before  returns
 */
uint8_t* srv_DoHandshake(int sockfd, char* dhparam_path) {
    uint8_t* result = NULL;
    DH* dh;

    dh = srv_LoadDHParam(dhparam_path);
    if(!dh) {
        goto cleanup;
    }
    if(DH_generate_key(dh) == 0) {
        LogEvent(LOG_ERROR, "DH_generate_key() failed : %s", strerror(errno));
        goto dh_cleanup;
    }
    uint32_t dhsize = DH_size(dh);
    if(dhsize != 0x20) {
        LogEvent(LOG_ERROR, "Incorrect DH_size()");
        goto dh_cleanup;
    }
    uint8_t* pubkey = malloc(dhsize);
    if(!pubkey) {
        LogEvent(LOG_ERROR, "malloc() oom?");
        goto dh_cleanup;
    }
    if(BN_bn2bin(dh->pub_key,pubkey) == 0) {
        LogEvent(LOG_ERROR, "BN_bn2bin() failed");
        goto pk_cleanup;
    }
    if(writeAll(sockfd, pubkey, dhsize) < 0) {
        goto pk_cleanup;
    }
    if(readAll(sockfd, pubkey, dhsize) < 0) {
        goto pk_cleanup;
    }
    BIGNUM* bn = BN_bin2bn(pubkey, dhsize, NULL);
    if(!bn) {
        goto pk_cleanup;
    }
    if(!DH_compute_key(pubkey, bn, dh)) {
        LogEvent(LOG_ERROR, "DH_compute_key() failed");
        goto pk_cleanup;
    }

    result = malloc(0x10);
    memcpy(result, pubkey, 0x10);
    memcpy(g_IV, pubkey + 0x10, 0x10);
pk_cleanup:
    free(pubkey);
dh_cleanup:
    DH_free(dh);
cleanup:
    return result;
}

#ifdef _CRYPTO_SUPPORT
/*
static int srv_ReadPacketInternal(int sockfd, uint8_t* buffer, uint32_t length, uint8_t* key) {
    uint32_t readLen = ROUNDUP16(length);
    uint8_t* tmp = malloc(readLen);
    if(!tmp) {
        LogEvent(LOG_ERROR, "malloc() oom? readLen = %u", readLen);
        return -1;
    }
    if(readAll(sockfd, tmp, readLen) < 0) {
        free(tmp);
        return -1;
    }
    int result = AES128_cbc(tmp, readLen, buffer, length, key, g_IV, 0);
    free(tmp);
    return result;
}

uint8_t* srv_ReadPacket(int sockfd, uint32_t maxsize, uint32_t* key) {
    uint32_t readSize;
    int result = srv_ReadPacketInternal(sockfd, &readSize, 4, key);
    if(result != 4 || readSize > 0x10000) {
        return NULL;
    }
    uint8_t* buffer = malloc(readSize);
    result = srv_ReadPacket(sockfd, buffer, readSize, key);
    if(result < 0) {
        free(buffer);
        return NULL;
    }
    return buffer;
}
*/

uint8_t* srv_ReadPacket(int sockfd, uint32_t maxsize, uint32_t* key) {
    uint32_t readSize;
    uint8_t* tmp = malloc(0x10);
    uint8_t* buffer = NULL;
    if(!tmp) {
        LogEvent(LOG_ERROR, "malloc() oom?");
        return NULL;
    }
    int result = readAll(sockfd, tmp, 0x10);
    if(result < 0) {
        free(tmp);
        return NULL;
    }
    result = AES128_cbc(tmp, 0x10, &readSize, sizeof(readSize), key, g_IV, 0);
    LogEvent(LOG_INFO, "Incoming packet size = %d", readSize);
    if(result != 4 || readSize >= MAX_PACKET) {
        free(tmp);
        return NULL;
    }
    tmp = realloc(tmp, readSize + 1);
    if(!tmp) {
        LogEvent(LOG_ERROR, "malloc() oom?");
        return NULL;
    }
    uint32_t bufSize = maxsize < readSize ? maxsize : readSize;
    buffer = realloc(NULL, bufSize);
    if(!buffer) {
        LogEvent(LOG_ERROR, "malloc() oom?");
        free(tmp);
        return NULL;
    }
    result = readAll(sockfd, tmp, readSize);
    if(result < 0) {
        free(tmp);
        return NULL;
    }
    result = AES128_cbc(tmp, readSize, buffer, bufSize, key, g_IV, 0);
    if(result < 0) {
        free(tmp);
        free(buffer);
        return NULL;
    }
    free(tmp);
    return buffer;
}

int srv_WritePacket(int sockfd, uint8_t* packet, uint32_t size, uint8_t* key) {
    uint8_t* tmp;
    uint32_t sendSize = ROUNDUP16(size);

    if(sendSize == size)    sendSize += 0x10;   // ex-padding
    tmp = malloc(0x10);
    if(!tmp) {
        LogEvent(LOG_ERROR, "malloc() oom?");
        return -1;
    }
    LogEvent(LOG_INFO, "Sending packet size = %d", sendSize);
    int result = AES128_cbc(&sendSize, 4, tmp, 0x10, key, g_IV, 1);
    if(result != 0x10) {
        LogEvent(LOG_ERROR, "AES128_cbc() failed");
        free(tmp);
        return -1;
    }
    result = writeAll(sockfd, tmp, 0x10);
    if(result < 0) {
        free(tmp);
        return -1;
    }
    tmp = realloc(tmp, sendSize + 1);
    if(!tmp) {
        LogEvent(LOG_ERROR, "malloc() oom?");
        return -1;
    }
    result = AES128_cbc(packet, size, tmp, sendSize, key, g_IV, 1);
    if(result != sendSize) {
        LogEvent(LOG_ERROR, "AES128_cbc() failed");
        free(tmp);
        return -1;
    }
    result = writeAll(sockfd, tmp, sendSize);
    free(tmp);
    return result;
}
#else
uint8_t* srv_ReadPacket(int sockfd, uint32_t maxsize, uint32_t* key) {
    uint32_t readSize;
    if(readAll(sockfd, &readSize, sizeof(readSize)) < 0) {
        return NULL;
    }
    if(readSize > 0x10000)  {   // length check
        return NULL;
    }
    uint8_t* buffer = malloc(readSize);
    if(readAll(sockfd, buffer, readSize) < 0) {
        free(buffer);
        return NULL;
    } 
    return buffer;
}
#endif  // _CRYPTO_SUPPORT

int srv_TCPConnect(char* address, uint16_t port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockfd < 0) {
        LogEvent(LOG_ERROR, "socket() failed : %s", __ERRSTR__);
        return -1;
    }
    struct sockaddr_in host;
    bzero(&host, sizeof(host));
    host.sin_family = AF_INET;
    host.sin_port = htons(port);
    host.sin_addr.s_addr = inet_addr(address);
    if(connect(sockfd, (struct sockaddr*)&host, sizeof(host)) < 0) {
        LogEvent(LOG_ERROR, "connect() failed : %s", __ERRSTR__);
        close(sockfd);
        return -1;
    }
    return sockfd;
}

int srv_TCPListenConnectBack(uint16_t port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockfd < 0) {
        LogEvent(LOG_ERROR, "socket() failed : %s", __ERRSTR__);
        return -1;
    }

    int reuse = 1;
    if( setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0 ) {
        LogEvent(LOG_WARNING, "setsockopt() failed : %s", strerror(errno));
    }
    struct sockaddr_in host;
    bzero(&host, sizeof(host));
    host.sin_family = AF_INET;
    host.sin_port = htons(port);
    host.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(sockfd, (struct sockaddr_in*)&host, sizeof(host)) < 0) {
        LogEvent(LOG_ERROR, "bind() failed : %s", __ERRSTR__);
        close(sockfd);
        return -1;
    }
    if(listen(sockfd, 5) < 0) {
        LogEvent(LOG_ERROR, "listen() failed : %s", __ERRSTR__);
        close(sockfd);
        return -1;
    }
    return sockfd;
}

void* srv_FileTransferThread(void* args) {
    int sockfd = *(int*)args;
    uint8_t* buffer = *(uint8_t**)((uint8_t*)args + sizeof(int));
    uint32_t size = *(uint32_t*)((uint8_t*)args + sizeof(int)+sizeof(uint8_t*));
    struct sockaddr_in clin;
    uint32_t clinlen = sizeof(clin);
    int clisock = accept(sockfd, &clin, &clinlen);
    if(clisock < 0) {
        close(sockfd);
        return NULL;
    }
    close(sockfd);
    writeAll(clisock, buffer, size);

    close(clisock);
    return NULL;
}

int srv_TCPStartFileTransfer(uint16_t port, char* filepath, uint32_t size, pthread_t* rthread, int* rsocket) {
    pthread_t thread;
    int fd = open(filepath, O_RDONLY);
    if(fd < 0) {
        LogEvent(LOG_ERROR, "open() failed : %s", __ERRSTR__);
        return -1;
    }
    uint8_t* buffer = malloc(size);
    if(!buffer) {
        close(fd);
        return -1;
    }
    if(readAll(fd, buffer, size) < 0) {
        close(fd);
        return -1;
    }
    close(fd);

    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockfd < 0) {
        LogEvent(LOG_ERROR, "socket() failed : %s", __ERRSTR__);
        return -1;
    }

    int reuse = 1;
    if( setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0 ) {
        LogEvent(LOG_WARNING, "setsockopt() failed : %s", strerror(errno));
    }
    struct sockaddr_in host;
    bzero(&host, sizeof(host));
    host.sin_family = AF_INET;
    host.sin_port = htons(port);
    host.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(sockfd, (struct sockaddr_in*)&host, sizeof(host)) < 0) {
        LogEvent(LOG_ERROR, "bind() failed : %s", __ERRSTR__);
        close(sockfd);
        return -1;
    }
    if(listen(sockfd, 1) < 0) {
        LogEvent(LOG_ERROR, "listen() failed : %s", __ERRSTR__);
        close(sockfd);
        return -1;
    }
    uint8_t* args = malloc(sizeof(int) + sizeof(uint8_t*) + sizeof(uint32_t));
    *(int*)args = sockfd;
    *(uint8_t**)(args+sizeof(int)) = buffer;
    *(uint32_t*)(args+sizeof(int)+sizeof(uint8_t*)) = size;
    pthread_create(&thread, NULL, srv_FileTransferThread, args);
    *rthread = thread;
    *rsocket = sockfd;

    return 0;
}

int srv_TCPFileTransfer(in_addr_t addr, uint16_t port, uint32_t size, char* path) {
    /* TODO : add openssl support */
    int result = -1;
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockfd < 0) {
        LogEvent(LOG_ERROR, "socket() failed : %s", __ERRSTR__);
        return -1;
    }
    struct sockaddr_in sin;

    bzero(&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = addr;

    if(connect(sockfd, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
        LogEvent(LOG_ERROR, "connect() failed : %s", __ERRSTR__);
        close(sockfd);
        return -1;
    }

    uint8_t* buffer = malloc(size);
    if(buffer == NULL) {
        LogEvent(LOG_ERROR, "malloc() oom");
        close(sockfd);
        return -1;
    }
    result = readAll(sockfd, buffer, size);
    close(sockfd);

    if(result < 0) {
        free(buffer);
        return result;
    }

    int fd = open(path, O_WRONLY | O_CREAT, 0755);
    if(fd < 0) {
        LogEvent(LOG_ERROR, "open() failed : %s", __ERRSTR__);
        free(buffer);
        return -1;
    }

    result = writeAll(fd, buffer, size);
    free(buffer);
    return result;
}
