#ifndef _COMMON_H
#error "Do NOT include this header solely."
#endif

#define _CRYPTO_SUPPORT     // added crypto support

#define MAX_PACKET 0x10000

/* server handler prototype */
typedef void* (*srv_handler)(void* arg);
typedef int (*srv_udp_handler)(int sockfd, uint8_t* buffer, uint32_t len, struct sockaddr_in* sin);

/* functions */
int srv_StartListen(uint16_t port);
void __attribute__((noreturn)) srv_ServeForever(int sockfd, srv_handler handler);

int srv_StartUDPBinding(uint16_t port);
void __attribute__((noreturn)) srv_ServeUDPForever(int sockfd, srv_udp_handler handler);

uint8_t* srv_DoHandshake(int sockfd, char* dhparam_path, uint8_t** iv);

uint8_t* srv_ReadPacket(int sockfd, uint32_t maxsize, uint32_t* key, uint8_t* iv);
int srv_WritePacket(int sockfd, uint8_t* packet, uint32_t size, uint8_t* key, uint8_t* iv);

int srv_TCPConnectBack(in_addr_t addr, uint16_t port);