#ifndef _COMMON_H
#error "Do NOT include this header solely."
#endif

#define PTYPE_RESULT    0x4b5f5f4b

/*
 *  Register 
 */

#define PTYPE_REGISTER   0x1
#define PTYPE_REGRESULT  0x81
#define PTYPE_LOGIN      0x2
#define PTYPE_LOGINRESULT 0x82
#define PTYPE_MESSAGE    0x3
#define PTYPE_MSGRESULT  0x83
#define PTYPE_SENDFILE   0x4
#define PTYPE_SFILERESULT 0x84
#define PTYPE_GETFILE   0x5
#define PTYPE_GFILERESULT 0x85
#define PTYPE_FOPDONE   0x6
#define PTYPE_CHANGENAME  0x7
#define PTYPE_CNRESULT  0x87
#define PTYPE_GETNAME   0x8
#define PTYPE_GNRESULT  0x88
#define PTYPE_CREATEGROUP 0x9
#define PTYPE_CGRESULT  0x89
#define PTYPE_ADDFRIENDREQ     0xa
#define PTYPE_AFRESULT  0x8a
#define PTYPE_BEFRIEND  0xb
#define PTYPE_BFRESULT  0x8b
#define PTYPE_QUERYFRIEND 0xc
#define PTYPE_QFRESULT  0x8c
#define PTYPE_DELETEFRIEND 0xd
#define PTYPE_DFRESULT  0x8d
#define PTYPE_GETGROUPS 0xe
#define PTYPE_GGRESULT  0x8e
#define PTYPE_GETGROUPUSERS     0xf
#define PTYPE_GGURESULT  0x8f
#define PTYPE_GROUPMSG  0x10
#define PTYPE_GMRESULT  0x90
#define PTYPE_GETAVATAR 	0x11
#define PTYPE_GARESULT 		0x91
#define PTYPE_SETAVATAR     0x12
#define PTYPE_SARESULT      0x92

typedef struct _REQ_REGISTER {
    char uname[32];     // max length
    char passwd[32];    // passwd
} REQ_REGISTER, *PREQ_REGISTER;

typedef struct _RES_REGISTER {
    uint32_t result;
    uint32_t uid;
} RES_REGISTER, *PRESRES_REGISTER;

typedef struct _REQ_LOGIN {
    uint32_t uid;
    uint16_t port;
    char passwd[32];
} REQ_LOGIN, *PREQ_LOGIN;

typedef struct _RES_LOGIN {
    uint32_t result;
    uint32_t login_session;
} RES_LOGIN, *PRES_LOGIN;

typedef struct _REQ_MESSAGE {
    uint32_t to_uid;
    uint32_t msglen;
} REQ_MESSAGE, *PREQ_MESSAGE;

typedef struct _RES_MESSAGE {
    uint32_t result;
} RES_MESSAGE, *PRES_MESSAGE;

/* send file */

#define MAX_FILE 0x1e00000

typedef struct _REQ_SENDFILE {
    uint32_t filesize;
    uint32_t to_uid;
    char filename[40];
} REQ_SENDFILE, *PREQ_SENDFILE;

typedef struct _REQ_GETFILE {
    uint32_t dummy;
} REQ_GETFILE, *PGETFILE;

typedef struct _RES_GETFILE {
    uint32_t result;
    in_addr_t ipaddr;
    uint32_t port;
} RES_GETFILE, *PRES_GETFILE;

typedef struct _RES_SENDFILE {
    uint32_t result;
} RES_SENDFILE, *PRES_SENDFILE;

typedef struct _FOP_DONE {
    uint32_t result;
    uint32_t uid;
} FOP_DONE, *PFOP_DONE;

typedef struct _REQ_CHANGENAME {
    char username[32];
} REQ_CHANGENAME, *PREQ_CHANGENAME;

typedef struct _RES_CHANGENAME {
    uint32_t result;
} RES_CHANGENAME, *PRES_CHANGENAME;

typedef struct _REQ_GETNAME {
    uint32_t uid;
} REQ_GETNAME, *PREQ_GETNAME;

typedef struct _RES_GETNAME {
    uint32_t result;
    char username[32];
} RES_GETNAME, *PRES_GETNAME;

typedef struct _REQ_CREATEGROUP {
    uint32_t usercount;
} REQ_CREATEGROUP, *PREQ_CREATEGROUP;

typedef struct _RES_CREATEGROUP {
    uint32_t result;
    uint32_t gid;
} RES_CREATEGROUP, *PRES_CREATEGROUP;

typedef struct _REQ_ADDFRIEND {
    uint32_t uid;
} REQ_ADDFRIEND, *PREQ_ADDFRIEND;

typedef struct _RES_ADDFREIND {
    uint32_t result;
} RES_ADDFRIEND, *PRES_ADDFRIEND;

typedef struct _REQ_BEFRIEND {
    uint32_t uid;
} REQ_BEFRIEND, *PREQ_BEFRIEND;

typedef struct _RES_BEFRIEND {
    uint32_t result;
} RES_BEFRIEND, *PRES_BEFRIEND;

typedef struct _REQ_QUERYFRIEND {
    uint32_t dummy;
} REQ_QUERYFRIEND, PREQ_QUERYFRIEND;

typedef struct _RES_QUERYFRIEND {
    uint32_t result;
    uint32_t friend_count;
} RES_QUERYFRIEND, *PRES_QUERYFRIEND;

typedef struct _REQ_DELETEFRIEND {
    uint32_t uid;
} REQ_DELETEFRIEND, *PREQ_DELETEFRIEND;

typedef struct _RES_DELETEFRIEND {
    uint32_t result;
} RES_DELETEFRIEND, *PRES_DELETEFRIEND;

typedef struct _REQ_GETGROUPS {
    uint32_t dummy;
} REQ_GETGROUPS, *PREQ_GETGROUPS;

typedef struct _RES_GETGROUPS {
    uint32_t result;
    uint32_t groupcnt;
} RES_GETGROUPS, *PRES_GETGROUPS;

typedef struct _REQ_GETGROUPUSERS {
    uint32_t gid;
} REQ_GETGROUPUSERS, *PREQ_GETGROUPUSERS;

typedef struct _RES_GETGROUPUSERS {
    uint32_t result;
    uint32_t usercount;
} RES_GETGROUPUSERS, *PRES_GETGROUPUSERS;

typedef struct _REQ_GROUPMSG {
    uint32_t gid;
    uint32_t msglen;
} REQ_GROUPMSG, *PREQ_GROUPMSG;

typedef struct _RES_GROUPMSG {
    uint32_t result;
} RES_GROUPMSG, *PRES_GROUPMSG;

typedef struct _REQ_GETAVATAR {
	uint32_t uid;
} REQ_GETAVATAR, *PREQ_GETAVATAR;

typedef struct _RES_GETAVATAR {
	uint32_t result;
	uint32_t avatar_id;
} RES_GETAVATAR, *PRES_GETAVATAR;

typedef struct _REQ_SETAVATAR {
	uint32_t avatar_id;
} REQ_SETAVATAR, *PREQ_SETAVATAR;

typedef struct _RES_SETAVATAR {
	uint32_t result;
} RES_SETAVATAR, *PRES_SETAVATAR;

/* message */ 
#define USERMSG_MAGIC   'MESG'
#define USERFILE_MAGIC  'FILE'
#define USERLOG_MAGIC   'ONLI'
#define USERNAME_MAGIC  'NAME'  // following packet : newname
#define USERFRIEND_MAGIC 'DAWG'
#define USERBEFRIEND_MAGIC 'HELO'
#define USERDELFRIEND_MAGIC     'DIED'
#define USERNEWGROUP_MAGIC 'GRUP'
#define USERGROUPMSG_MAGIC 'GMSG'
#define USERFILECANCEL_MAGIC 'CANC'

typedef struct _USER_MSG {
    uint32_t magic;
    uint32_t uid;
    uint32_t msglen;
} USER_MSG, *PUSER_MSG;

typedef struct _UMSG_FILE {
    uint32_t filesz;
    char filename[40];
} UMSG_FILE, *PUMSG_FILE;

typedef struct _UMSG_LOGIN {
    uint32_t disconnect;
    uint32_t uid;
    in_addr_t ipaddr;
} UMSG_LOGIN, *PUMSG_LOGIN;

typedef struct _UMSG_GROUPMSG {
    uint32_t gid;
    uint8_t msg[1];     // variable-sized
} UMSG_GROUPMSG, *PUMSG_GROUPMSG;

#define PACKET_MAGIC    'PACK'

typedef struct _PACKET {
    uint32_t magic;
    uint32_t timestamp;
    uint32_t ptype;
    union {
        /* register */
        REQ_REGISTER regreq;   // register
        RES_REGISTER regres;

        /* login */
        REQ_LOGIN logreq;
        RES_LOGIN logres; 

        /* message */
        REQ_MESSAGE msgreq;
        RES_MESSAGE msgres;

        /* sendfile */
        REQ_SENDFILE filereq;
        RES_SENDFILE fileres;

        /* getfile */
        REQ_GETFILE gfilereq;
        RES_GETFILE gfileres;

        /* fopdone */
        FOP_DONE fopdone;

        /* change name */
        REQ_CHANGENAME cnreq;
        RES_CHANGENAME cnres;

        /* get name */
        REQ_GETNAME gnreq;
        RES_GETNAME gnres;

        /* create group */
        REQ_CREATEGROUP creategreq;
        RES_CREATEGROUP creategres;

        /* add friend req */
        REQ_ADDFRIEND afreq;
        RES_ADDFRIEND afres;

        /* befriend */
        REQ_BEFRIEND bfreq;
        RES_BEFRIEND bfres;

        /* query friends */
        REQ_QUERYFRIEND qfreq;
        RES_QUERYFRIEND qfres;

        /* delete friends */
        REQ_DELETEFRIEND dfreq;
        RES_DELETEFRIEND dfres;

        /* get groups */
        REQ_GETGROUPS ggreq;
        RES_GETGROUPS ggres;

        /* get group users */
        REQ_GETGROUPUSERS ggureq;
        RES_GETGROUPUSERS ggures;

        /* group msg */
        REQ_GROUPMSG gmsgreq;
        RES_GROUPMSG gmsgres;

	    /* get avatar */
	    REQ_GETAVATAR gareq;
	    RES_GETAVATAR gares;

        /* set avatar */
        REQ_SETAVATAR sareq;
        RES_SETAVATAR sares;
    } u;
} PACKET, *PPACKET;


#define CLIENT_MSG_MAGIC    'CMSG'

typedef struct _CLIENT_MSG {
    uint32_t magic;
    int sockfd;     // tcp socket 
    uint32_t majorfunc;     // command
    union {
        struct {
            uint32_t mm_len;
            uint8_t* mm_msg;   //  formatted multi-media msg
            /* TODO : add font data */ 
        } msg;
        struct {
            char* filepath;
        } file;
        struct {
            uint8_t data[1];
        } raw;
        /* TODO : add more msg union */
    } u;
} CLIENT_MSG, *PCLIENT_MSG;

#define NICK_MAX 25
#define DISCLOSURE_MAGIC    'CSGO'

/* UDP Protocol */
typedef struct _HOST_INFO {
    uint32_t magic;
    //uint32_t ipaddr;
    uint8_t nickname[NICK_MAX];   // max length nickname length

    uint32_t checksum;      // md5 of the packet
} HOST_INFO, *PHOST_INFO;

/* APIs */
PPACKET protocol_BuildRegisterPacket(char* name, char* passwd);
uint32_t protocol_ParseRegResultPacket(PPACKET packet, uint32_t* uid);

PPACKET protocol_BuildLoginPacket(uint32_t uid, char* passwd);
uint32_t protocol_ParseLogResultPacket(PPACKET packet, uint32_t* session);

uint32_t protocol_ParseMsgResultPacket(PPACKET packet);
PPACKET protocol_BuildMsgPacket(uint32_t uid, uint32_t msglen);

uint32_t protocol_ParseSFileResultPacket(PPACKET packet);
PPACKET protocol_BuildSendFilePacket(uint32_t uid, char* filename, uint32_t filesize);
int protocol_GetFileInfo(char* path, uint32_t* size);

uint32_t protocol_ParseGFileResultPacket(PPACKET packet);
PPACKET protocol_BuildGetFilePacket(uint32_t dummy);

PPACKET protocol_BuildFopDonePacket(uint32_t result);

PPACKET protocol_BuildBeFriendPacket(uint32_t uid);
PPACKET protocol_BuildAddFriendPacket(uint32_t uid);
PPACKET protocol_BuildQueryFriendPacket(uint32_t dummy);
uint32_t protocol_ParseBFResultPacket(PPACKET packet);
uint32_t protocol_ParseAFResultPacket(PPACKET packet);
uint32_t protocol_ParseQFResultPacket(PPACKET packet, uint32_t* fcount);
PPACKET protocol_BuildQueryFriendPacket(uint32_t uid);
uint32_t protocol_ParseDFResultPacket(PPACKET packet);
PPACKET protocol_BuildDeleteFriendPacket(uint32_t uid);

uint32_t protocol_ParseGNResultPacket(PPACKET packet);
PPACKET protocol_BuildGetNamePacket(uint32_t uid);
PPACKET protocol_BuildChangeNamePacket(char* name);
uint32_t protocol_ParseCNResultPacket(PPACKET packet);

uint32_t protocol_ParseCGResultPacket(PPACKET packet, uint32_t* gid);
PPACKET protocol_BuildCreateGroupPacket(uint32_t usercount);
PPACKET protocol_BuildGetGroupsPacket(uint32_t dummy);
PPACKET protocol_BuildGetGroupUsersPacket(uint32_t gid);
uint32_t protocol_ParseGGResultPacket(PPACKET packet, uint32_t* groupcnt);
uint32_t protocol_ParseGGUResultPacket(PPACKET packet, uint32_t* usercount);
PPACKET protocol_BuildGroupMsgPacket(uint32_t gid, uint32_t msglen);
uint32_t protocol_ParseGMResultPacket(PPACKET packet);

/* msg handler */
void* __attribute__((noreturn)) client_msg_handler(void* args);

void addChatHistory(int uid1, int uid2, int send, char* msg);
void addGroupHistory(int gid, int uid1, int uid2, int send, char* msg);

int addFriendReq(int uid);
int beFriend(int uid);
int groupMsg(int gid, char* msg);
int createGroup(uint32_t usercount, uint32_t* user, uint32_t* gid);
int queryGroup(uint32_t uid, uint32_t* groupcount, uint32_t** groups);
int queryGroupUsers(uint32_t gid, uint32_t* usercount, uint32_t** users);
int sendFile(uint32_t uid, char* filename, uint32_t filesize, char* filepath);
int getFile(char* path);
int getAvatar(uint32_t uid, uint32_t* avatar_id);
int setAvatar(uint32_t avatar_id);
int delFriend(int uid);
