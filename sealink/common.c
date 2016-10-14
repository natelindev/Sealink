#include "common.h"
#include <openssl/aes.h>
#include <openssl/md5.h>

static FILE* g_errFile = NULL;
static char* g_logLevel[] = {
    "DBUG",
    "INFO",
    "WARN",
    "ERRO"
};

void __attribute__((noreturn)) errPrint(char* msg) {
    perror(msg);
    exit(-1);
}

/* Initialization */
void LogInit(char* logpath) {
    g_errFile = fopen(logpath, "a+");
    if(!g_errFile) {
        errPrint("fopen");
    }
    return;
}



/*
 * LogEvent - log events
 * args :
 *      level - log level
 *      fmt - format string
 * return :
 *      no return
 * N.B. :
 *      initialize at first
 */
void LogEvent(int level, char* fmt, ...) {
    va_list args;
    struct timeval tv;
    struct tm tm;
    char buf[BUF_SIZE];

    gettimeofday(&tv, NULL);
    if( g_errFile == NULL ) {     // wtf no log file ???
        return;
    }
    if( level > MAX_LOG_LEVEL || level <= MIN_LOG_LEVEL ) {   // sanity check
        return;
    }
    va_start(args, fmt);
    if( localtime_r(&tv.tv_sec, &tm) ) {
        strftime(buf, BUF_SIZE, "[%%s] %T : ", &tm);
        fprintf(g_errFile, buf, g_logLevel[level]);
        vfprintf(g_errFile, fmt, args);
        fputc('\n', g_errFile);
    }

    fflush(g_errFile);
    va_end(args);
    return;
}

int readAll(int fd, uint8_t* buffer, uint32_t len) {
    uint8_t* p = buffer;
    while(len) {
        int r = read(fd, p, len);
        if( r <= 0 ) {  // read failed
            LogEvent(LOG_WARNING, "readAll() error during transmission : %d %s", r, __ERRSTR__);
            return -1; 
        }
        p += r;
        len -= r;
    }
    return 0;
}

int writeAll(int fd, uint8_t* buffer, uint32_t len) {
    uint8_t* p = buffer;
    while(len) {
        int w = write(fd, p, len);
        if( w <= 0 ) {  // write failed
            LogEvent(LOG_WARNING, "writeAll() error during transmission : %d %s", w, __ERRSTR__);
            return -1;
        }
        p += w;
        len -= w;
    }
    return 0;
}

char* parseConfig(char* filePath, char* option) {
    FILE* fp;
    char* result = NULL;
    char buffer[512];
    char optname[512], optvalue[512];
    fp = fopen(filePath, "r");
    if(!fp) {
        LogEvent(LOG_ERROR, "parseConfig() : cannot open file");
        return NULL;
    }
    while(1) {
        if(fgets(buffer,512,fp) == NULL)   break;
        if(sscanf(buffer, "%s = %s", optname, optvalue)!=2)     continue;
        if(strcasecmp(optname,option) == 0) {
            result = strdup(optvalue);
            break;
        }
    }

out_cleanup:
    fclose(fp);
    return result;
}

// MD5
//---------------------------------------------------------------------
static void tohex32(unsigned char* data,char* buffer,unsigned long len)
{
	int i;
	char tmp[20];
	buffer[0] = 0;
	for(i=0;i<16;i++)
	{
		snprintf(tmp,20,"%02x",data[i]);
		strncat(buffer,tmp,len);
	}
	return;
}

#ifdef _STATIC_MD5
typedef struct _MD5_CTX
{
    unsigned int count[2];
    unsigned int state[4];
    unsigned char buffer[64];   
}MD5_CTX;

#define F(x,y,z) ((x & y) | (~x & z))
#define G(x,y,z) ((x & z) | (y & ~z))
#define H(x,y,z) (x^y^z)
#define I(x,y,z) (y ^ (x | ~z))
#define ROTATE_LEFT(x,n) ((x << n) | (x >> (32-n)))

#define FF(a,b,c,d,x,s,ac) \
          { \
          a += F(b,c,d) + x + ac; \
          a = ROTATE_LEFT(a,s); \
          a += b; \
          }

#define GG(a,b,c,d,x,s,ac) \
          { \
          a += G(b,c,d) + x + ac; \
          a = ROTATE_LEFT(a,s); \
          a += b; \
          }

#define HH(a,b,c,d,x,s,ac) \
          { \
          a += H(b,c,d) + x + ac; \
          a = ROTATE_LEFT(a,s); \
          a += b; \
          }

#define II(a,b,c,d,x,s,ac) \
          { \
          a += I(b,c,d) + x + ac; \
          a = ROTATE_LEFT(a,s); \
          a += b; \
          }                                

static void MD5Encode(unsigned char *output,unsigned int *input,unsigned int len);
static void MD5Update(MD5_CTX *context,unsigned char *input,unsigned int inputlen);
static void MD5Final(MD5_CTX *context,unsigned char digest[16]);
static void MD5Encode(unsigned char *output,unsigned int *input,unsigned int len);
static void MD5Decode(unsigned int *output,unsigned char *input,unsigned int len);
static void MD5Transform(unsigned int state[4],unsigned char block[64]);

unsigned char PADDING[]={0x80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static void MD5Init(MD5_CTX *context)
{
     context->count[0] = 0;
     context->count[1] = 0;
     context->state[0] = 0x67452301;
     context->state[1] = 0xEFCDAB89;
     context->state[2] = 0x98BADCFE;
     context->state[3] = 0x10325476;
     return;
}
static void MD5Update(MD5_CTX *context,unsigned char *input,unsigned int inputlen)
{
    unsigned int i = 0,index = 0,partlen = 0;
    index = (context->count[0] >> 3) & 0x3F;
    partlen = 64 - index;
    context->count[0] += inputlen << 3;
    if(context->count[0] < (inputlen << 3))
       context->count[1]++;
    context->count[1] += inputlen >> 29;
    
    if(inputlen >= partlen)
    {
       memcpy(&context->buffer[index],input,partlen);
       MD5Transform(context->state,context->buffer);
       for(i = partlen;i+64 <= inputlen;i+=64)
           MD5Transform(context->state,&input[i]);
       index = 0;        
    }  
    else
    {
        i = 0;
    }
    memcpy(&context->buffer[index],&input[i],inputlen-i);
    return;
}

static void MD5Final(MD5_CTX *context,unsigned char digest[16])
{
    unsigned int index = 0,padlen = 0;
    unsigned char bits[8];
    index = (context->count[0] >> 3) & 0x3F;
    padlen = (index < 56)?(56-index):(120-index);
    MD5Encode(bits,context->count,8);
    MD5Update(context,PADDING,padlen);
    MD5Update(context,bits,8);
    MD5Encode(digest,context->state,16);
    return;
}
static void MD5Encode(unsigned char *output,unsigned int *input,unsigned int len)
{
    unsigned int i = 0,j = 0;
    while(j < len)
    {
         output[j] = input[i] & 0xFF;  
         output[j+1] = (input[i] >> 8) & 0xFF;
         output[j+2] = (input[i] >> 16) & 0xFF;
         output[j+3] = (input[i] >> 24) & 0xFF;
         i++;
         j+=4;
    }
    return;
}
static void MD5Decode(unsigned int *output,unsigned char *input,unsigned int len)
{
     unsigned int i = 0,j = 0;
     while(j < len)
     {
           output[i] = (input[j]) |
                       (input[j+1] << 8) |
                       (input[j+2] << 16) |
                       (input[j+3] << 24);
           i++;
           j+=4; 
     }
     return;
}
static void MD5Transform(unsigned int state[4],unsigned char block[64])
{
     unsigned int a = state[0];
     unsigned int b = state[1];
     unsigned int c = state[2];
     unsigned int d = state[3];
     unsigned int x[64];
     MD5Decode(x,block,64);
     FF(a, b, c, d, x[ 0], 7, 0xd76aa478); /* 1 */
	 FF(d, a, b, c, x[ 1], 12, 0xe8c7b756); /* 2 */
	 FF(c, d, a, b, x[ 2], 17, 0x242070db); /* 3 */
	 FF(b, c, d, a, x[ 3], 22, 0xc1bdceee); /* 4 */
	 FF(a, b, c, d, x[ 4], 7, 0xf57c0faf); /* 5 */
	 FF(d, a, b, c, x[ 5], 12, 0x4787c62a); /* 6 */
	 FF(c, d, a, b, x[ 6], 17, 0xa8304613); /* 7 */
	 FF(b, c, d, a, x[ 7], 22, 0xfd469501); /* 8 */
	 FF(a, b, c, d, x[ 8], 7, 0x698098d8); /* 9 */
	 FF(d, a, b, c, x[ 9], 12, 0x8b44f7af); /* 10 */
	 FF(c, d, a, b, x[10], 17, 0xffff5bb1); /* 11 */
	 FF(b, c, d, a, x[11], 22, 0x895cd7be); /* 12 */
	 FF(a, b, c, d, x[12], 7, 0x6b901122); /* 13 */
	 FF(d, a, b, c, x[13], 12, 0xfd987193); /* 14 */
	 FF(c, d, a, b, x[14], 17, 0xa679438e); /* 15 */
	 FF(b, c, d, a, x[15], 22, 0x49b40821); /* 16 */
	 
	 /* Round 2 */
	 GG(a, b, c, d, x[ 1], 5, 0xf61e2562); /* 17 */
	 GG(d, a, b, c, x[ 6], 9, 0xc040b340); /* 18 */
	 GG(c, d, a, b, x[11], 14, 0x265e5a51); /* 19 */
	 GG(b, c, d, a, x[ 0], 20, 0xe9b6c7aa); /* 20 */
	 GG(a, b, c, d, x[ 5], 5, 0xd62f105d); /* 21 */
	 GG(d, a, b, c, x[10], 9,  0x2441453); /* 22 */
	 GG(c, d, a, b, x[15], 14, 0xd8a1e681); /* 23 */
	 GG(b, c, d, a, x[ 4], 20, 0xe7d3fbc8); /* 24 */
	 GG(a, b, c, d, x[ 9], 5, 0x21e1cde6); /* 25 */
	 GG(d, a, b, c, x[14], 9, 0xc33707d6); /* 26 */
	 GG(c, d, a, b, x[ 3], 14, 0xf4d50d87); /* 27 */
	 GG(b, c, d, a, x[ 8], 20, 0x455a14ed); /* 28 */
	 GG(a, b, c, d, x[13], 5, 0xa9e3e905); /* 29 */
	 GG(d, a, b, c, x[ 2], 9, 0xfcefa3f8); /* 30 */
	 GG(c, d, a, b, x[ 7], 14, 0x676f02d9); /* 31 */
	 GG(b, c, d, a, x[12], 20, 0x8d2a4c8a); /* 32 */
	 
	 /* Round 3 */
	 HH(a, b, c, d, x[ 5], 4, 0xfffa3942); /* 33 */
	 HH(d, a, b, c, x[ 8], 11, 0x8771f681); /* 34 */
	 HH(c, d, a, b, x[11], 16, 0x6d9d6122); /* 35 */
	 HH(b, c, d, a, x[14], 23, 0xfde5380c); /* 36 */
	 HH(a, b, c, d, x[ 1], 4, 0xa4beea44); /* 37 */
	 HH(d, a, b, c, x[ 4], 11, 0x4bdecfa9); /* 38 */
	 HH(c, d, a, b, x[ 7], 16, 0xf6bb4b60); /* 39 */
	 HH(b, c, d, a, x[10], 23, 0xbebfbc70); /* 40 */
	 HH(a, b, c, d, x[13], 4, 0x289b7ec6); /* 41 */
	 HH(d, a, b, c, x[ 0], 11, 0xeaa127fa); /* 42 */
	 HH(c, d, a, b, x[ 3], 16, 0xd4ef3085); /* 43 */
	 HH(b, c, d, a, x[ 6], 23,  0x4881d05); /* 44 */
	 HH(a, b, c, d, x[ 9], 4, 0xd9d4d039); /* 45 */
	 HH(d, a, b, c, x[12], 11, 0xe6db99e5); /* 46 */
	 HH(c, d, a, b, x[15], 16, 0x1fa27cf8); /* 47 */
	 HH(b, c, d, a, x[ 2], 23, 0xc4ac5665); /* 48 */
	 
	 /* Round 4 */
	 II(a, b, c, d, x[ 0], 6, 0xf4292244); /* 49 */
	 II(d, a, b, c, x[ 7], 10, 0x432aff97); /* 50 */
	 II(c, d, a, b, x[14], 15, 0xab9423a7); /* 51 */
	 II(b, c, d, a, x[ 5], 21, 0xfc93a039); /* 52 */
	 II(a, b, c, d, x[12], 6, 0x655b59c3); /* 53 */
	 II(d, a, b, c, x[ 3], 10, 0x8f0ccc92); /* 54 */
	 II(c, d, a, b, x[10], 15, 0xffeff47d); /* 55 */
	 II(b, c, d, a, x[ 1], 21, 0x85845dd1); /* 56 */
	 II(a, b, c, d, x[ 8], 6, 0x6fa87e4f); /* 57 */
	 II(d, a, b, c, x[15], 10, 0xfe2ce6e0); /* 58 */
	 II(c, d, a, b, x[ 6], 15, 0xa3014314); /* 59 */
	 II(b, c, d, a, x[13], 21, 0x4e0811a1); /* 60 */
	 II(a, b, c, d, x[ 4], 6, 0xf7537e82); /* 61 */
	 II(d, a, b, c, x[11], 10, 0xbd3af235); /* 62 */
	 II(c, d, a, b, x[ 2], 15, 0x2ad7d2bb); /* 63 */
	 II(b, c, d, a, x[ 9], 21, 0xeb86d391); /* 64 */
     state[0] += a;
     state[1] += b;
     state[2] += c;
     state[3] += d;
     return;
}

void md5(unsigned char* data,unsigned long len,unsigned char* buffer,unsigned long buf_len)
{
	unsigned char hexdata[30];
	MD5_CTX md5_context;

	MD5Init(&md5_context);
	MD5Update(&md5_context,data,len);
	MD5Final(&md5_context,hexdata);
	tohex32(hexdata,buffer,buf_len);
	return;
}
#else
void md5(unsigned char* data,unsigned long len,unsigned char* buffer,unsigned long buf_len)
{
    uint8_t md5result[20];
    
    MD5(data,len,md5result);
    tohex32(md5result,buffer,buf_len);
    return;
}
#endif
//---------------------------------------------------------------------

/*
 * AES128_cbc - aes128 cbc using pkcs5 padding
 */
int AES128_cbc(uint8_t* buffer, uint32_t len, uint8_t* output, uint32_t maxlen, uint8_t* key, uint8_t* iv, uint32_t encrypt)
{
    int result = -1, i;
    uint8_t* padded = NULL, padlen;
    uint32_t padsize;
    AES_KEY aeskey;
    if(encrypt) {   // encrypt
        if(AES_set_encrypt_key(key,128,&aeskey))    return -1;
        padsize = ROUNDUP16(len);
        if(padsize == len)  padsize += 16;  // full block padding
        padded = malloc(padsize);
        if(!padded)     return -1;
        memcpy(padded,buffer,len);
        for(i=len;i<padsize;i++)    padded[i] = padsize - len;
        if(padsize > maxlen)    padsize = maxlen;
        AES_cbc_encrypt(padded,output,padsize,&aeskey,iv,1);
    } else {
        if(AES_set_decrypt_key(key,128,&aeskey))    return -1;
        if(len & 0xf)   return -1;
        padded = malloc(len);
        if(!padded)     return -1;
        AES_cbc_encrypt(buffer,padded,len,&aeskey,iv,0);
        padlen = padded[len-1];
        if(padlen>0x10)     return -1;  // wrong padding
        /* TODO : PERFORM MORE SANITY CHECK FOR PADDING LENGTH */
        if(padlen + maxlen < len)   len = maxlen + padlen;  // buffer overflow check
        memcpy(output,padded,len-padlen);
        padsize = len - padlen;
    }
    free(padded);
    return padsize;
}

uint32_t getRandom32()
{
    int fd;
    uint32_t result;
    if((fd = open("/dev/urandom",O_RDONLY))<0)  return -1;
    read(fd,&result,4);
    close(fd);
    return result;
}

char* pathToFileName(char* path) {
    char* p;

    p = path + strlen(path) - 1;
    while(p > path) {
        if(*p == '/') {
            return (p+1);
        }
        p--;
    }
    return p;
}