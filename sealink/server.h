#ifndef _COMMON_H
#error "Do NOT include this header solely."
#endif

#define _CRYPTO_SUPPORT

#define MAX_PACKET 0x10000

/* server handler prototype */
typedef void* (*srv_handler)(void* arg);
typedef int (*srv_udp_handler)(int sockfd, uint8_t* buffer, uint32_t len, struct sockaddr_in* sin);

/* functions */
int srv_StartListen(uint16_t port);
void __attribute__((noreturn)) srv_ServeForever(int sockfd, srv_handler handler);

int srv_StartUDPBinding(uint16_t port);
void __attribute__((noreturn)) srv_ServeUDPForever(int sockfd, srv_udp_handler handler);

uint8_t* srv_DoHandshake(int sockfd, char* dhparam_path);

uint8_t* srv_ReadPacket(int sockfd, uint32_t maxsize, uint32_t* key);
int srv_WritePacket(int sockfd, uint8_t* packet, uint32_t size, uint8_t* key);

int srv_TCPConnect(char* address, uint16_t port);
int srv_TCPListenConnectBack(uint16_t port);
int srv_TCPFileTransfer(in_addr_t addr, uint16_t port, uint32_t size, char* path);

int srv_TCPStartFileTransfer(uint16_t port, char* filepath, uint32_t size, pthread_t* rthread, int* rsocket);
