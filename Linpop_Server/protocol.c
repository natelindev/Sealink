#include "common.h"

/* Register */

PPACKET protocol_BuildRegisterResult(uint32_t result, uint32_t uid) {
    /* TODO : query uid */ 
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet) {
        LogEvent(LOG_ERROR, "malloc() oom?");
        return NULL;
    }

    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_REGRESULT;
    packet->u.regres.result = result;
    packet->u.regres.uid = uid;
    return packet;
}

int protocol_Register(PPACKET packet) {
    int result = -1;
    if(packet->magic != PACKET_MAGIC) {
        LogEvent(LOG_ERROR, "Malformed packet");
        return result;
    }
    if(packet->ptype != PTYPE_REGISTER) {
        LogEvent(LOG_ERROR, "Not register event");
        return result;
    }
    MYSQL* conn = db_EstablishConnection(DB_HOST, DB_PORT, DB_USERNAME, DB_PASSWORD, DB_MAINDB);
    if(!conn) {
        LogEvent(LOG_ERROR, "connect to mysql failed");
        return result;
    }
    result = db_InsertUser(conn, packet->u.regreq.uname, packet->u.regreq.passwd);

    mysql_close(conn);
    return result;
}

/* Login */

PPACKET protocol_BuildLoginResult(uint32_t result, uint32_t login_session) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet) {
        LogEvent(LOG_ERROR, "malloc() oom?");
        return NULL;
    }

    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_LOGINRESULT;
    packet->u.logres.result = result;
    packet->u.logres.login_session = login_session;
    return packet;
}

int protocol_Login(PPACKET packet) {
    int result = -1;

    /* TODO : Add re-login check */
    if(packet->magic != PACKET_MAGIC) {
        LogEvent(LOG_ERROR, "Malformed packet");
        return result;
    }
    if(packet->ptype != PTYPE_LOGIN) {
        LogEvent(LOG_ERROR, "Not login event");
        return result;
    }
    MYSQL* conn = db_EstablishConnection(DB_HOST, DB_PORT, DB_USERNAME, DB_PASSWORD, DB_MAINDB);
    if(!conn) {
        LogEvent(LOG_ERROR, "connect to mysql failed");
        return result;
    }
    result = db_LoginCheck(conn, packet->u.logreq.uid, packet->u.logreq.passwd);
    if(result >= 0) {   // login success
        /* TODO : add login session handling */
    }
    mysql_close(conn);
    return result;
}

/* Message */

PPACKET protocol_BuildMessageResult(uint32_t result) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet) {
        LogEvent(LOG_ERROR, "malloc() oom?");
        return NULL;
    }

    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_MSGRESULT;
    packet->u.msgres.result = result;

    return packet;
}

int protocol_Message(PPACKET packet, PCLIENT client, int sockfd, uint8_t* key, uint8_t* iv) {
    int result = -1;
    PCLIENT target;

    if(!client || !packet)  return result;
    if(packet->magic  != PACKET_MAGIC || packet->ptype != PTYPE_MESSAGE)    return result;
    target = login_FindOnlineClientByUID(packet->u.msgreq.to_uid);
    if(!target) {   // not online
        LogEvent(LOG_ERROR, "UID %d not online", packet->u.msgreq.to_uid);
        return result;
    }
    uint8_t* buffer = srv_ReadPacket(sockfd, packet->u.msgreq.msglen, key, iv);
    if(!buffer) {
        LogEvent(LOG_ERROR, "readPacket() cannot read message");
        return result;
    }
    PUSER_MSG msg = malloc(sizeof(USER_MSG));
    msg->magic = USERMSG_MAGIC;
    msg->msglen = packet->u.msgreq.msglen;
    msg->uid = client->uid;
    result = srv_WritePacket(target->msgsock, msg, sizeof(USER_MSG), target->key, target->iv);
    if(result < 0) {
        LogEvent(LOG_ERROR, "writePacket() failed to write");
        free(buffer);
        free(msg);
        return result;
    }
    result = srv_WritePacket(target->msgsock, buffer, msg->msglen, target->key, target->iv);
    if(result < 0) {
        LogEvent(LOG_ERROR, "writePacket() failed to write");
        free(buffer);
        free(msg);
        return result;
    }

    free(buffer);
    free(msg);
    result = 0;
    return result;
}

PPACKET protocol_BuildSendFileResult(uint32_t result) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet) {
        LogEvent(LOG_ERROR, "malloc() oom?");
        return NULL;
    }

    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_SFILERESULT;
    packet->u.fileres.result = result;

    return packet;
}

int protocol_SendFile(PPACKET packet, PCLIENT client, int sockfd) {
    int result = -1;
    if(packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_SENDFILE)    return result;
    PCLIENT target;
    target = login_FindOnlineClientByUID(packet->u.filereq.to_uid);
    if(!target) {
        LogEvent(LOG_ERROR, "User(%d) offline", packet->u.filereq.to_uid);
        return -1;
    }
    if(packet->u.filereq.filesize > MAX_FILE) {
        LogEvent(LOG_ERROR, "File too big (%x)", packet->u.filereq.filesize);
        return -1;
    }
    PFILE_REQUEST freq = malloc(sizeof(FILE_REQUEST));
    freq->from_uid = client->uid;
    freq->filesz = packet->u.filereq.filesize;
    freq->filename = strdup(packet->u.filereq.filename);
    if(login_AddFileReq(target, freq) < 0) {
        LogEvent(LOG_ERROR, "File transfer in progress");
        free(freq);
        return -1;
    } 

    /* notify target */
    PUSER_MSG umsg = malloc(sizeof(USER_MSG));
    umsg->magic = USERFILE_MAGIC;
    umsg->uid = client->uid;
    umsg->msglen = sizeof(UMSG_FILE);

    int w = srv_WritePacket(target->msgsock, umsg, sizeof(USER_MSG), target->key, target->iv);
    if(w < 0) {
        LogEvent(LOG_ERROR, "writePacket() failed");
        free(umsg);
        return -1;
    }
    free(umsg);

    PUMSG_FILE mfile = malloc(sizeof(UMSG_FILE));
    mfile->filesz = packet->u.filereq.filesize;
    strncpy(mfile->filename, freq->filename, 40);
    w = srv_WritePacket(target->msgsock, mfile, sizeof(UMSG_FILE), target->key, target->iv);
    if(w < 0) {
        LogEvent(LOG_ERROR, "writePacket() failed");
        free(mfile);
        return -1;
    }
    free(mfile);

    result = 0;
    
    /*
    uint8_t* buffer = malloc(packet->u.filereq.filesize);
    if(!buffer) {
        LogEvent(LOG_ERROR, "malloc() oom");
        return -1;
    }
    uint32_t remain = packet->u.filereq.filesize;
    uint32_t packet_size;
    uint8_t* tmpbuf, *p = buffer;
    while(remain) {     // read file
        tmpbuf = srv_ReadPacket(sockfd, MAX_PACKET, client->key, client->iv);
        if(!tmpbuf) {
            LogEvent(LOG_ERROR, "Error during transmission");
            free(buffer);
            free(tmpbuf);
            return result;
        }
        packet_size = *(uint32_t*)tmpbuf;
        free(tmpbuf);
        LogEvent(LOG_DEBUG, "Transfer size = %u", packet_size);
        if(packet_size > MAX_PACKET || packet_size > remain)   {
            LogEvent(LOG_ERROR, "Invalid packet size");
            free(buffer);
            return result;
        }
        tmpbuf = srv_ReadPacket(sockfd, packet_size, client->key, client->iv);
        if(!tmpbuf) {
            LogEvent(LOG_ERROR, "Error during transmission");
            free(buffer);
            free(tmpbuf);
            return result;
        }
        memcpy(p, tmpbuf, packet_size);
        remain -= packet_size;
        p += packet_size;
        free(tmpbuf);
    }
    result = 
    */
    return result;
}

int protocol_GetFile(PPACKET packet, PCLIENT client) {
    if(packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_GETFILE){
        return -1;
    }
    if(!client->freq) {
        LogEvent(LOG_ERROR, "No file pending for (%d)", client->uid);
        return -1;
    }
    return 0;
}

PPACKET protocol_BuildGetFileResult(uint32_t result, in_addr_t addr, uint16_t port) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;
    
    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_GFILERESULT;
    packet->u.gfileres.result = result;
    packet->u.gfileres.ipaddr = addr;
    packet->u.gfileres.port = port;

    return packet;
}

uint32_t protocol_FopDone(PPACKET packet) {
    if(packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_FOPDONE){
        return -1;
    }
    if(packet->u.fopdone.result == 0xdeadbeef) {
        PCLIENT target = login_FindOnlineClientByUID(packet->u.fopdone.uid);
        if(target) {
            login_RemoveFileReq(target);
            PUSER_MSG msg = malloc(sizeof(USER_MSG));
            msg->magic = USERFILECANCEL_MAGIC;
            msg->uid = -1;
            msg->msglen = 0;
            srv_WritePacket(target->msgsock, msg, sizeof(USER_MSG), target->key, target->iv);
            free(msg);
        }
    } 
    return packet->u.fopdone.result;
}


int protocol_ChangeName(PPACKET packet, PCLIENT client) {
    if(!packet || !client)  return -1;
    if(packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_CHANGENAME)  return -1;

    MYSQL* conn = db_EstablishConnection(DB_HOST, DB_PORT, DB_USERNAME, DB_PASSWORD, DB_MAINDB);
    if(!conn) {
        LogEvent(LOG_ERROR, "connect to mysql failed");
        return -1;
    }
    int result = db_UpdateUsernameByUid(conn, client->uid, packet->u.cnreq.username);

    mysql_close(conn);

    return result;
}

PPACKET protocol_BuildCNResult(uint32_t result) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;
    
    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_CNRESULT;
    packet->u.cnres.result = result;
    return packet;
}

PPACKET protocol_BuildGNResult(uint32_t result, char* name) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;
    
    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_GNRESULT;
    packet->u.gnres.result = result;
    if(name) {
        strncpy(packet->u.gnres.username, name, 31);
    }
    return packet;
}

PPACKET protocol_GetName(PPACKET packet) {
    if(!packet || packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_GETNAME)  return NULL;

    MYSQL* conn = db_EstablishConnection(DB_HOST, DB_PORT, DB_USERNAME, DB_PASSWORD, DB_MAINDB);
    if(!conn) {
        LogEvent(LOG_ERROR, "connect to mysql failed");
        return NULL;
    }
    char* result = db_GetUsernameByUid(conn, packet->u.gnreq.uid);
    if(!result) {
	 mysql_close(conn);
        return protocol_BuildGNResult(-1, NULL);
    }
    PPACKET p = protocol_BuildGNResult(0, result);
    mysql_close(conn);
    free(result);
    return p;
}

int protocol_AddFriend(PPACKET packet, PCLIENT client) {
    if(!packet || !client || packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_ADDFRIENDREQ)  return -1;
    PCLIENT target = login_FindOnlineClientByUID(packet->u.afreq.uid);
    if(!target) {
        LogEvent(LOG_INFO, "user (%d) not online", packet->u.afreq.uid);
        return -1;
    }
    PUSER_MSG msg = malloc(sizeof(USER_MSG));
    msg->magic = USERFRIEND_MAGIC;
    msg->msglen = 0;
    msg->uid = client->uid;
    int result = srv_WritePacket(target->msgsock, msg, sizeof(USER_MSG), target->key, target->iv);
    free(msg);
    return result;
}

PPACKET protocol_BuildAFResult(uint32_t result) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;
    
    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_AFRESULT;
    packet->u.afres.result = result;
    return packet;
}

PPACKET protocol_BuildBFResult(uint32_t result) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;
    
    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_BFRESULT;
    packet->u.bfres.result = result;
    return packet;
}

int protocol_BeFriend(PPACKET packet, PCLIENT client) {
    if(!packet || !client || packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_BEFRIEND)  return -1;
    PCLIENT target = login_FindOnlineClientByUID(packet->u.bfreq.uid);
    if(!target) {
        LogEvent(LOG_INFO, "user (%d) not online", packet->u.afreq.uid);
        return -1;
    }
    MYSQL* conn = db_EstablishConnection(DB_HOST, DB_PORT, DB_USERNAME, DB_PASSWORD, DB_MAINDB);
    if(!conn) {
        LogEvent(LOG_ERROR, "connect to mysql failed");
        return -1;
    }
    int result = db_AddFriends(conn, client->uid, packet->u.afreq.uid);
    if(result < 0) {
        LogEvent(LOG_ERROR, "add friend failed");
        mysql_close(conn);
        return -1;
    }
    mysql_close(conn);

    PUSER_MSG msg = malloc(sizeof(USER_MSG));
    msg->magic = USERBEFRIEND_MAGIC;
    msg->msglen = 0;
    msg->uid = client->uid; 
    result = srv_WritePacket(target->msgsock, msg, sizeof(USER_MSG), target->key, target->iv);
    free(msg);
    
    return 0;
}

PPACKET protocol_BuildQFResult(uint32_t result, uint32_t fcount) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;
    
    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_QFRESULT;
    packet->u.qfres.result = result;
    packet->u.qfres.friend_count = fcount;
    return packet;
}

int protocol_QueryFriend(PPACKET packet, PCLIENT client, uint32_t* fcount, uint32_t** friends) {
    if(!packet || !client || packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_QUERYFRIEND)  return -1;
    MYSQL* conn = db_EstablishConnection(DB_HOST, DB_PORT, DB_USERNAME, DB_PASSWORD, DB_MAINDB);
    if(!conn) {
        LogEvent(LOG_ERROR, "connect to mysql failed");
        return -1;
    }
    int result = db_QueryFriends(conn, client->uid, fcount, friends);
    if(result < 0) {
        LogEvent(LOG_ERROR, "query friend failed");
        mysql_close(conn);
        return -1;
    }
    mysql_close(conn);
    return 0;
}

int protocol_DeleteFriend(PPACKET packet, PCLIENT client) {
    if(!packet || !client || packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_DELETEFRIEND)  return -1;
    PCLIENT target = login_FindOnlineClientByUID(packet->u.dfreq.uid);
    MYSQL* conn = db_EstablishConnection(DB_HOST, DB_PORT, DB_USERNAME, DB_PASSWORD, DB_MAINDB);
    if(!conn) {
        LogEvent(LOG_ERROR, "connect to mysql failed");
        return -1;
    }
    int result = db_DeleteFriend(conn, client->uid, packet->u.dfreq.uid);
    if(result >= 0 && target) {
        PUSER_MSG msg = malloc(sizeof(USER_MSG));
        msg->magic = USERDELFRIEND_MAGIC;
        msg->msglen = 0;
        msg->uid = client->uid;
        srv_WritePacket(target->msgsock, msg, sizeof(USER_MSG), target->key, target->iv);
    }
    mysql_close(conn);
    return result;
}

PPACKET protocol_BuildDFResult(uint32_t result) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;
    
    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_DFRESULT;
    packet->u.dfres.result = result;
    return packet;
}

PPACKET protocol_BuildCGResult(uint32_t result, uint32_t gid) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;
    
    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_CGRESULT;
    packet->u.creategres.result = result;
    packet->u.creategres.gid = gid;
    return packet;
}

int protocol_CreateGroup(PPACKET packet, PCLIENT client, int sockfd, uint32_t* rgid) {
    uint32_t gid;

    if(!packet || !client || packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_CREATEGROUP)  return -1;
    /* TODO : gid sanity check */
    gid = getRandom32();
    uint32_t uc = packet->u.creategreq.usercount;
    if(uc > 10) return -1;  // no more than 10 

    MYSQL* conn = db_EstablishConnection(DB_HOST, DB_PORT, DB_USERNAME, DB_PASSWORD, DB_MAINDB);
    if(!conn) {
        LogEvent(LOG_ERROR, "connect to mysql failed");
        return -1;
    }

    uint32_t* buffer = srv_ReadPacket(sockfd, uc*sizeof(uint32_t), client->key, client->iv);
    if(!buffer) {
        LogEvent(LOG_ERROR, "Transmission error");
        mysql_close(conn);
        return -1;
    }

    LogEvent(LOG_DEBUG, "UserGroup (%u) with user count (%d)", gid, uc);
    int result = db_CreateGroup(conn, gid, uc, buffer);
    if(result < 0) {
        free(buffer);
        mysql_close(conn);
        return -1;
    }

    uint32_t i;
    for(i=0;i<uc;i++) {
        if(client->uid == buffer[i])     continue;
        login_Notify(client->uid, buffer[i], USERNEWGROUP_MAGIC, &gid, 4);
    }

    *rgid = gid;
    free(buffer);
    mysql_close(conn);
    return 0;
}

PPACKET protocol_BuildGGResult(uint32_t result, uint32_t groupcnt) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;
    
    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_GGRESULT;
    packet->u.ggres.result = result;
    packet->u.ggres.groupcnt = groupcnt;

    return packet;
}

int protocol_GetGroups(PPACKET packet, PCLIENT client, uint32_t* groupcnt, uint32_t** groups) {
    if(!packet || !client || packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_GETGROUPS)     return -1;

    MYSQL* conn = db_EstablishConnection(DB_HOST, DB_PORT, DB_USERNAME, DB_PASSWORD, DB_MAINDB);
    if(!conn) {
        LogEvent(LOG_ERROR, "connect to mysql failed");
        return -1;
    }
    int result = db_GetUserGroups(conn, client->uid, groupcnt, groups);
    if(result < 0) {
        LogEvent(LOG_ERROR, "cannot get (%u) groups", client->uid);
        mysql_close(conn);
        return -1;
    }

    mysql_close(conn);
    return 0;
}

int protocol_GetGroupUsers(PPACKET packet, PCLIENT client, uint32_t* usercount, uint32_t** users) {
    if(!packet || !client || packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_GETGROUPUSERS)     return -1;
    MYSQL* conn = db_EstablishConnection(DB_HOST, DB_PORT, DB_USERNAME, DB_PASSWORD, DB_MAINDB);
    if(!conn) {
        LogEvent(LOG_ERROR, "connect to mysql failed");
        return -1;
    }
    int result = db_GetGroup(conn, packet->u.ggureq.gid, usercount, users);
    if(result < 0) {
        LogEvent(LOG_ERROR, "cannot get (%u) group users", packet->u.ggureq.gid);
        mysql_close(conn);
        return -1;
    }

    mysql_close(conn);
    return 0;
}

PPACKET protocol_BuildGGUResult(uint32_t result, uint32_t usercount) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;
    
    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_GGURESULT;
    packet->u.ggures.result = result;
    packet->u.ggures.usercount = usercount;

    return packet;
}

int protocol_GroupMessage(PPACKET packet, PCLIENT client, int sockfd) {
    if(!packet || !client || packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_GROUPMSG)     return -1;


    uint32_t usercount, *users;

    if(packet->u.gmsgreq.msglen > 0x10000) return -1;   // message too long
    MYSQL* conn = db_EstablishConnection(DB_HOST, DB_PORT, DB_USERNAME, DB_PASSWORD, DB_MAINDB);
    if(!conn) {
        LogEvent(LOG_ERROR, "connect to mysql failed");
        return -1;
    }
    int result = db_GetGroup(conn, packet->u.gmsgreq.gid, &usercount, &users);
    if(result < 0) {
        LogEvent(LOG_ERROR, "cannot get (%u) group users", packet->u.gmsgreq.gid);
        mysql_close(conn);
        return -1;
    }
    mysql_close(conn);

    uint8_t* message = srv_ReadPacket(sockfd, packet->u.gmsgreq.msglen, client->key, client->iv);   // MEM DISCLOSURE HERE!!!
    if(!message) {
        LogEvent(LOG_ERROR, "cannot get message");
        free(users);
        return -1;
    }

    PUMSG_GROUPMSG buffer = malloc(packet->u.gmsgreq.msglen + sizeof(uint32_t));
    buffer->gid = packet->u.gmsgreq.gid;
    memcpy(buffer->msg, message, packet->u.gmsgreq.msglen);

    uint32_t i;
    for(i=0;i<usercount;i++) {
        if(client->uid == users[i])     continue;
        login_Notify(client->uid, users[i], USERGROUPMSG_MAGIC, buffer, packet->u.gmsgreq.msglen + sizeof(uint32_t));
    }

    free(buffer);
    free(users);
    return 0;
}

PPACKET protocol_BuildGMResult(uint32_t result) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;
    
    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_GMRESULT;
    packet->u.gmsgres.result = result;
    return packet;
}

PPACKET protocol_BuildGAResult(uint32_t result, uint32_t avatar_id) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;
    
    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_GARESULT;
    packet->u.gares.result = result;
    packet->u.gares.avatar_id = avatar_id;
    return packet;
}

PPACKET protocol_BuildSAResult(uint32_t result) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;
    
    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_SARESULT;
    packet->u.sares.result = result;
    return packet;
}

int protocol_GetAvatar(PPACKET packet, uint32_t* avatar_id) {
    if(!packet || packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_GETAVATAR)     return -1;

    MYSQL* conn = db_EstablishConnection(DB_HOST, DB_PORT, DB_USERNAME, DB_PASSWORD, DB_MAINDB);
    if(!conn) {
        LogEvent(LOG_ERROR, "connect to mysql failed");
        return -1;
    }
    int result = db_GetAvatar(conn, packet->u.gareq.uid, avatar_id);
    mysql_close(conn);
    return result;
}

int protocol_SetAvatar(PPACKET packet, PCLIENT client) {
    if(!packet || !client || packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_SETAVATAR)     return -1;

    MYSQL* conn = db_EstablishConnection(DB_HOST, DB_PORT, DB_USERNAME, DB_PASSWORD, DB_MAINDB);
    if(!conn) {
        LogEvent(LOG_ERROR, "connect to mysql failed");
        return -1;
    }
    int result = db_UpdateAvatar(conn, client->uid, packet->u.sareq.avatar_id);

    mysql_close(conn);

    return result;
}
