#ifndef _COMMON_H
#define _COMMON_H

#define _GNU_SOURCE
/*
 * Common headers
 */ 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/types.h>
#include <fcntl.h>

#include <mysql.h>

#define MAX_LOG_LEVEL 3
#define MIN_LOG_LEVEL -1

#define LOG_DEBUG   0
#define LOG_INFO    1
#define LOG_WARNING 2
#define LOG_ERROR   3

#define __ERRSTR__ (strerror(errno))

/* Macros */
#define ROUNDUP16(x) ((((uint32_t)x)+16-1)&(~(16-1)))

/* Initializers */
void LogInit(char* logpath);

/* Util functions */
void __attribute__((noreturn)) errPrint(char* msg);
void LogEvent(int level, char* fmt, ...);
int readAll(int fd, uint8_t* buffer, uint32_t len);
int writeAll(int fd, uint8_t* buffer, uint32_t len);
char* parseConfig(char* filePath, char* option);
void md5(unsigned char* data,unsigned long len,unsigned char* buffer,unsigned long buf_len);
int AES128_cbc(uint8_t* buffer, uint32_t len, uint8_t* output, uint32_t maxlen, uint8_t* key, uint8_t* iv, uint32_t encrypt);
uint32_t getRandom32();
int urandom(uint8_t* buffer, uint32_t length);

/* Options */
#define SERVER_PORT 0x1337
#define CLIENT_PORT 0x9009
#define FILET_PORT  0xfeed
#define LOG_PATH    "/tmp/linpop_server.log"
#define BUF_SIZE    0x1000

/* User-defined */
#include "server.h"
#include "login.h"
#include "protocol.h"
#include "dbconn.h"
//#include "frelay.h"

#endif