#include "common.h"
#include <gtk/gtk.h>
#include "chat_window.h"
#include "global_settings.h"
#include "callbacks.h"
#include "user_dao.h"
#include "notification.h"
#include "global_settings.h"

extern uint32_t g_uid;
extern uint8_t* key;
extern int g_sockfd;

pthread_t g_SendFileThread;
int g_SendFileSocket;

PPACKET protocol_BuildRegisterPacket(char* name, char* passwd) {

    if(strlen(name) > 32 || strlen(passwd) > 32)   return NULL;
    PPACKET result = malloc(sizeof(PACKET));
    if(!result) {
        LogEvent(LOG_ERROR, "malloc() oom?");
        return NULL;
    }

    bzero(result, sizeof(PACKET));
    result->magic = PACKET_MAGIC;
    result->ptype = PTYPE_REGISTER;
    strncpy(result->u.regreq.uname, name, 31);
    strncpy(result->u.regreq.passwd, passwd, 31);

    return result;
}

uint32_t protocol_ParseRegResultPacket(PPACKET packet, uint32_t* uid) {
    if(packet->magic != PACKET_MAGIC)   return -1;
    if(packet->ptype != PTYPE_REGRESULT)    return -1;
    *uid = packet->u.regres.uid;
    return packet->u.regres.result;
}

PPACKET protocol_BuildLoginPacket(uint32_t uid, char* passwd) {

    if(strlen(passwd) > 32)     return NULL;
    PPACKET result = malloc(sizeof(PACKET));
    if(!result) {
        LogEvent(LOG_ERROR, "malloc() oom?");
        return NULL;
    }

    bzero(result, sizeof(PACKET));
    result->magic = PACKET_MAGIC;
    result->ptype = PTYPE_LOGIN;
    result->u.logreq.uid = uid;
    strncpy(result->u.logreq.passwd, passwd, 31);
    
    return result;
}

uint32_t protocol_ParseLogResultPacket(PPACKET packet, uint32_t* session) {
    if(packet->magic != PACKET_MAGIC)   return -1;
    if(packet->ptype != PTYPE_LOGINRESULT)    return -1;
    *session = packet->u.logres.login_session;
    return packet->u.logres.result;
}

uint32_t protocol_ParseMsgResultPacket(PPACKET packet) {
    if(packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_MSGRESULT)   return -1;
    return packet->u.msgres.result;
}

PPACKET protocol_BuildMsgPacket(uint32_t uid, uint32_t msglen) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;

    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_MESSAGE;
    packet->u.msgreq.to_uid = uid;
    packet->u.msgreq.msglen = msglen;
    return packet;
}

uint32_t protocol_ParseSFileResultPacket(PPACKET packet) {
    if(packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_SFILERESULT)     return -1;
    return packet->u.fileres.result;
}

PPACKET protocol_BuildSendFilePacket(uint32_t uid, char* filename, uint32_t filesize) {
    if(strlen(filename) > 40)   return NULL;
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;

    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_SENDFILE;
    packet->u.filereq.to_uid = uid;
    packet->u.filereq.filesize = filesize;
    strncpy(packet->u.filereq.filename, filename, 40);

    return packet;
}

int protocol_GetFileInfo(char* path, uint32_t* size) {

    if(access(path, R_OK) < 0) {
        LogEvent(LOG_ERROR, "access() failed : %s", __ERRSTR__);
        return -1;
    }
    struct stat filestat;
    if(stat(path, &filestat) < 0) {
        LogEvent(LOG_ERROR, "stat() failed : %s", __ERRSTR__);
        return -1;
    }
    *size = filestat.st_size;
    return 0;
}

uint32_t protocol_ParseGFileResultPacket(PPACKET packet) {
    if(packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_GFILERESULT)     return -1;
    return packet->u.gfileres.result;
}

PPACKET protocol_BuildGetFilePacket(uint32_t dummy) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;

    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_GETFILE;
    packet->u.gfilereq.dummy = dummy;
    return packet;
}

PPACKET protocol_BuildFopDonePacket(uint32_t result) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;

    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_FOPDONE;
    packet->u.fopdone.result = result;
    return packet;
}

PPACKET protocol_BuildAddFriendPacket(uint32_t uid) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;

    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_ADDFRIENDREQ;
    packet->u.afreq.uid = uid;
    return packet;
}

PPACKET protocol_BuildBeFriendPacket(uint32_t uid) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;

    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_BEFRIEND;
    packet->u.bfreq.uid = uid;
    return packet;
}

PPACKET protocol_BuildQueryFriendPacket(uint32_t dummy) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;

    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_QUERYFRIEND;
    packet->u.qfreq.dummy = dummy;
    return packet;
}

uint32_t protocol_ParseBFResultPacket(PPACKET packet) {
    if(packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_BFRESULT)     return -1;
    return packet->u.bfres.result;
}

uint32_t protocol_ParseAFResultPacket(PPACKET packet) {
    if(packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_AFRESULT)     return -1;
    return packet->u.afres.result;
}

uint32_t protocol_ParseQFResultPacket(PPACKET packet, uint32_t* fcount) {
    if(packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_QFRESULT)     return -1;
    *fcount = packet->u.qfres.friend_count;
    return packet->u.qfres.result;
}

uint32_t protocol_ParseDFResultPacket(PPACKET packet) {
    if(packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_DFRESULT)     return -1;
    return packet->u.dfres.result;
}

PPACKET protocol_BuildDeleteFriendPacket(uint32_t uid) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;

    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_DELETEFRIEND;
    packet->u.dfreq.uid = uid;
    return packet;
}

PPACKET protocol_BuildGetNamePacket(uint32_t uid) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;

    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_GETNAME;
    packet->u.gnreq.uid = uid;
    return packet; 
}

uint32_t protocol_ParseGNResultPacket(PPACKET packet) {
    if(packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_GNRESULT)     return -1;
    return packet->u.gnres.result;
}

PPACKET protocol_BuildChangeNamePacket(char* name) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;

    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_CHANGENAME;
    strncpy(packet->u.cnreq.username, name, 31);

    return packet;
}

uint32_t protocol_ParseCNResultPacket(PPACKET packet) {
    if(packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_CNRESULT)     return -1;
    return packet->u.cnres.result;
}

PPACKET protocol_BuildCreateGroupPacket(uint32_t usercount) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;

    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_CREATEGROUP;
    packet->u.creategreq.usercount = usercount;

    return packet;
}

uint32_t protocol_ParseCGResultPacket(PPACKET packet, uint32_t* gid) {
    if(packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_CGRESULT)     return -1;
    *gid = packet->u.creategres.gid;
    return packet->u.creategres.result;
}

PPACKET protocol_BuildGetGroupsPacket(uint32_t dummy) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;

    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_GETGROUPS;
    packet->u.ggreq.dummy = dummy;

    return packet;
}

PPACKET protocol_BuildGetGroupUsersPacket(uint32_t gid) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;

    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_GETGROUPUSERS;
    packet->u.ggureq.gid = gid;

    return packet;
}

uint32_t protocol_ParseGGResultPacket(PPACKET packet, uint32_t* groupcnt) {
    if(packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_GGRESULT)     return -1;
    *groupcnt = packet->u.ggres.groupcnt;
    return packet->u.ggres.result;
}

uint32_t protocol_ParseGGUResultPacket(PPACKET packet, uint32_t* usercount) {
    if(packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_GGURESULT)     return -1;
    *usercount = packet->u.ggures.usercount;
    return packet->u.ggures.result;
}

PPACKET protocol_BuildGroupMsgPacket(uint32_t gid, uint32_t msglen) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;

    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_GROUPMSG;
    packet->u.gmsgreq.gid = gid;
    packet->u.gmsgreq.msglen = msglen;

    return packet;
}

uint32_t protocol_ParseGAResultPacket(PPACKET packet, uint32_t* avatar_id) {
	if(packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_GARESULT)     return -1;
	*avatar_id = packet->u.gares.avatar_id;
	return packet->u.gares.result;
}

uint32_t protocol_ParseSAResultPacket(PPACKET packet) {
        if(packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_SARESULT)     return -1;
        return packet->u.gares.result;
}

PPACKET protocol_BuildGetAvatarPacket(uint32_t uid) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;

    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_GETAVATAR;
    packet->u.gareq.uid = uid;

    return packet;
}

PPACKET protocol_BuildSetAvatarPacket(uint32_t avatar_id) {
    PPACKET packet = malloc(sizeof(PACKET));
    if(!packet)     return NULL;

    bzero(packet, sizeof(PACKET));
    packet->magic = PACKET_MAGIC;
    packet->ptype = PTYPE_SETAVATAR;
    packet->u.sareq.avatar_id = avatar_id;

    return packet;
}

uint32_t protocol_ParseGMResultPacket(PPACKET packet) {
    if(packet->magic != PACKET_MAGIC || packet->ptype != PTYPE_GMRESULT)     return -1;

    return packet->u.gmsgres.result;
}

extern volatile int g_LoginReady;
extern volatile uint32_t g_FileSz;
extern uint8_t* key;

void addChatHistory(int uid1, int uid2, int send, char* msg) {
	char path[1024];
	snprintf(path, 1024, "%s/chat_history/%u_%u.chist", g_s.main_path, uid1, uid2);
	FILE* fp;
	fp = fopen(path, "a+");
	if(!fp) 	return;
	time_t tn = time(0);
	char name[100];
	if(!send) {
		QueryUserName(uid2, name);
		fprintf(fp, "%s & %s", name, ctime(&tn));
	} else {
		fprintf(fp, "Me & %s", ctime(&tn));
	}
	fprintf(fp, "%s\n", msg);

	fflush(fp);
	fclose(fp);
	return;
}

void addGroupHistory(int gid, int uid1, int uid2, int send, char* msg) {
	char path[1024];
	snprintf(path, 1024, "%s/chat_history/%u_group_%u.chist", g_s.main_path, uid1, gid);
	FILE* fp = fopen(path, "a+");
	if(!fp) 	return;
	time_t tn = time(0);
	char name[100];
	if(!send) {
		QueryUserName(uid2, name);
		fprintf(fp, "%s & %s", name, ctime(&tn));
	} else {
		fprintf(fp, "Me & %s", ctime(&tn));
	}
	fprintf(fp, "%s\n", msg);

	fflush(fp);
	fclose(fp);
	return;
}

void* __attribute__((noreturn)) client_msg_handler(void* args) {
	int bindsock = (int)args;
	struct sockaddr_in sin;
	uint32_t sinlen = sizeof(sin);
	int clisock;
	uint8_t* buffer;

	g_LoginReady = 1;
	clisock = accept(bindsock, &sin, &sinlen);
	if(clisock < 0) {
		LogEvent(LOG_ERROR, "FATAL : Connect back failed (%s)", __ERRSTR__);
		pthread_exit(-1);
	}

	close(bindsock); 	// only one

	LogEvent(LOG_INFO, "Server connected");
	while(1) {
		uint8_t* packet = srv_ReadPacket(clisock, MAX_PACKET, key);
		if(!packet) {
			LogEvent(LOG_INFO, "Server Disconnected");
			close(clisock);
			pthread_exit(-1);
		}
		PUSER_MSG msg = (PUSER_MSG)packet;
		if(msg->magic == USERMSG_MAGIC) { 	// msg recvd
			buffer = srv_ReadPacket(clisock, msg->msglen, key);
			if(!buffer) {
				LogEvent(LOG_INFO, "Transmission error");
				close(clisock);
				pthread_exit(-1);
			}

			printf("Message from (%d) : %s\n", msg->uid, buffer);
			int found = -1;
			GList* temp_win;
			chat_info* ci;
			temp_win = windows;
			while(temp_win != NULL) {
				ci = temp_win->data;
				if(ci->UID == msg->uid) {
					printf("Found in use window : %u\n", ci->UID);
					found = 0;
					break;
				}
				temp_win = g_list_next(temp_win);
			}
			uint32_t uname[50];

			if(found == 0) {
				char* t = malloc(msg->msglen + 0x100);
				memcpy(t, buffer, msg->msglen);
				t[msg->msglen] = 0;
				QueryUserName(msg->uid, uname);
				time_t tn = time(NULL);
				gdk_threads_enter();
				appendText(ci->receive_text, uname);
				appendText(ci->receive_text, " & ");
				appendText(ci->receive_text, ctime(&tn));
				appendText(ci->receive_text, t);
				appendText(ci->receive_text, "\n");
				gdk_threads_leave();
				free(t);
			} else { // pops up the window
				simple_user_info* sui = malloc(sizeof(simple_user_info));
				sui->UID = msg->uid;
				QueryUserName(msg->uid, sui->uname);
				addnotify(MSG, sui);
			}

			char* tmp = malloc(msg->msglen + 0x100);
			memcpy(tmp, buffer, msg->msglen);
			tmp[msg->msglen] = 0;

			addChatHistory(g_uid, msg->uid, 0, tmp);
			free(tmp);

			free(buffer);
		} else if(msg->magic == USERFILE_MAGIC) {
			buffer = srv_ReadPacket(clisock, msg->msglen, key);
			if(!buffer) {
				LogEvent(LOG_INFO, "Transmission error");
				close(clisock);
				pthread_exit(-1);
			}

			PUMSG_FILE mfile = (PUMSG_FILE)buffer;
			printf("Incoming file from (%d) : %s (%d)\n", msg->uid, mfile->filename, mfile->filesz);
			/* RACE CONDITION PREVENTION ? */
			g_FileSz = mfile->filesz;
			simple_user_info* sui = malloc(sizeof(simple_user_info));
			sui->UID = msg->uid;
			QueryUserName(msg->uid, sui->uname);
			addnotify(F_REC, sui);
			free(buffer);
		} else if(msg->magic == USERLOG_MAGIC) {
			buffer = srv_ReadPacket(clisock, msg->msglen, key);
			if(!buffer) {
				LogEvent(LOG_INFO, "Transmission error");
				close(clisock);
				pthread_exit(-1);
			}
			PUMSG_LOGIN mlog = (PUMSG_LOGIN)buffer;
			printf("User (%d) %s\n", mlog->uid, mlog->disconnect ? "disconnected" : "connected");

			simple_user_info* sui = malloc(sizeof(simple_user_info));
			sui->UID = mlog->uid;
			QueryUserName(mlog->uid, sui->uname);

			if(mlog->disconnect) 	addnotify(OFF, sui);
			else addnotify(ON, sui);

			free(buffer);
		} else if(msg->magic == USERNAME_MAGIC) {
			buffer = srv_ReadPacket(clisock, msg->msglen, key);
			if(!buffer) {
				LogEvent(LOG_INFO, "Transmission error");
				close(clisock);
				pthread_exit(-1);
			}
			printf("User (%d) name changed to %s\n", msg->uid, buffer);
			free(buffer);
		} else if(msg->magic == USERFRIEND_MAGIC) {
			printf("User (%d) wants to be friend with you\n", msg->uid);
		} else if(msg->magic == USERBEFRIEND_MAGIC) {
			printf("User (%d) has become a friend of yours\n", msg->uid);
		} else if(msg->magic == USERDELFRIEND_MAGIC) {
			printf("User (%d) has deleted you from his friend list\n", msg->uid);
		} else if(msg->magic == USERNEWGROUP_MAGIC) {
			buffer = srv_ReadPacket(clisock, msg->msglen, key);
			if(!buffer) {
				LogEvent(LOG_INFO, "Transmission error");
				close(clisock);
				pthread_exit(-1);
			}
			printf("User (%d) has invited you to group (%u)\n", msg->uid, *(uint32_t*)buffer);
			free(buffer);
		} else if(msg->magic == USERGROUPMSG_MAGIC) {
			buffer = srv_ReadPacket(clisock, msg->msglen, key);
			if(!buffer) {
				LogEvent(LOG_INFO, "Transmission error");
				close(clisock);
				pthread_exit(-1);
			}
			PUMSG_GROUPMSG gm = (PUMSG_GROUPMSG)buffer;
			printf("GroupMsg (%u) From user (%u) : %s\n", gm->gid, msg->uid, gm->msg);
			GList* tw;
			chat_info* gi;
			int gfound = -1;
			tw = group_windows;
			while(tw) {
				gi = tw->data;
				if((uint32_t)(gi->UID) == gm->gid) {
					printf("Found in use window\n");
					gfound = 0;
					break;
				}
				tw = g_list_next(tw);
			}
			char* ttt = malloc(msg->msglen-4 + 0x100);
			if( gfound == 0 ) {
				char username[50];
				QueryUserName(msg->uid, username);
				memcpy(ttt, gm->msg, msg->msglen - 4);
				ttt[msg->msglen-4] = 0;
				time_t tnn = time(NULL);
				gdk_threads_enter();
				appendText(gi->receive_text, username);
				appendText(gi->receive_text, " & ");
				appendText(gi->receive_text, ctime(&tnn));
				appendText(gi->receive_text, ttt);
				appendText(gi->receive_text, "\n");
				gdk_threads_leave();
			} else {
				simple_user_info* sui = malloc(sizeof(simple_user_info));
				sui->UID = gm->gid;
				addnotify(GMSG, sui);
			}
			addGroupHistory(gm->gid, g_uid, msg->uid, 0, ttt);
			free(ttt);
			free(buffer);
		}
		free(packet);
	}
}

int addFriendReq(int uid) {
	if(uid == g_uid) 	return -1;
	PPACKET packet = protocol_BuildAddFriendPacket(uid);
	if(!packet) 	return -1;
	int w = srv_WritePacket(g_sockfd, packet, sizeof(PACKET), key);
	free(packet);
	if( w < 0 ) 	return -1;
	PPACKET result = srv_ReadPacket(g_sockfd, MAX_PACKET, key);
	w = protocol_ParseAFResultPacket(result);
	free(result);
	return w;
}

int beFriend(int uid) {
	if(uid == g_uid) 	return -1;
	PPACKET packet = protocol_BuildBeFriendPacket(uid);
	if(!packet)     return -1;
        int w = srv_WritePacket(g_sockfd, packet, sizeof(PACKET), key);
        free(packet);
        if( w < 0 )     return -1;
        PPACKET result = srv_ReadPacket(g_sockfd, MAX_PACKET, key);
        w = protocol_ParseBFResultPacket(result);
        free(result);
        return w;	
}

int groupMsg(int gid, char* msg) {
	PPACKET packet = protocol_BuildGroupMsgPacket(gid, strlen(msg));
	if(!packet) 	return -1;
	int w = srv_WritePacket(g_sockfd, packet, sizeof(PACKET), key);
	free(packet);
	if( w < 0 )     return -1;
	w = srv_WritePacket(g_sockfd, msg, strlen(msg), key);
	if( w < 0 ) 	return -1;
	PPACKET result = srv_ReadPacket(g_sockfd, MAX_PACKET, key);
	w = protocol_ParseGMResultPacket(result);
	free(result);
	return w;
}

int createGroup(uint32_t usercount, uint32_t* user, uint32_t* gid)
{
	PPACKET packet = protocol_BuildCreateGroupPacket(usercount);
	if(!packet) 	return -1;
	int w = srv_WritePacket(g_sockfd, packet, sizeof(PACKET), key);
	free(packet);
	if( w < 0 ) 	return -1;
	w = srv_WritePacket(g_sockfd, user, usercount*sizeof(uint32_t), key);
	if( w < 0 ) 	return -1;
	PPACKET result = srv_ReadPacket(g_sockfd, MAX_PACKET, key);
	w = protocol_ParseCGResultPacket(result, gid);
	free(result);
	return w;
}

int queryGroup(uint32_t uid, uint32_t* groupcount, uint32_t** groups) {
	PPACKET packet = protocol_BuildGetGroupsPacket(uid);
	if(!packet) 	return -1;
	int w = srv_WritePacket(g_sockfd, packet, sizeof(PACKET), key);
	free(packet);
	if( w < 0 ) 	return -1;
	PPACKET result = srv_ReadPacket(g_sockfd, MAX_PACKET, key);
	if( !result ) 	return -1;
	w = protocol_ParseGGResultPacket(result, groupcount);
	free(result);
	if( w < 0 ) 	return -1;
	*groups = srv_ReadPacket(g_sockfd, sizeof(uint32_t)*(*groupcount), key);
	if( *groups ) 	return 0;
	return -1;
}

int queryGroupUsers(uint32_t gid, uint32_t* usercount, uint32_t** users) {
	PPACKET packet = protocol_BuildGetGroupUsersPacket(gid);
        if(!packet)     return -1;
        int w = srv_WritePacket(g_sockfd, packet, sizeof(PACKET), key);
        free(packet);
        if( w < 0 )     return -1;
        PPACKET result = srv_ReadPacket(g_sockfd, MAX_PACKET, key);
        if( !result )   return -1;
        w = protocol_ParseGGUResultPacket(result, usercount);
        free(result);
        if( w < 0 )     return -1;
        *users = srv_ReadPacket(g_sockfd, sizeof(uint32_t)*(*usercount), key);
        if( *users )   return 0;
        return -1;	
}

int sendFile(uint32_t uid, char* filename, uint32_t filesize, char* filepath) {
	PPACKET packet = protocol_BuildSendFilePacket(uid, filename, filesize);
	if(!packet) 	return -1;
	int w = srv_WritePacket(g_sockfd, packet, sizeof(PACKET), key);
	free(packet);
	if( w < 0 ) {
		printf("Error sending packet\n");
		return -1;
	}
	PPACKET result = srv_ReadPacket(g_sockfd, MAX_PACKET, key);
	if( result == 0 ) 	return -1;
	w = protocol_ParseSFileResultPacket(result);
	printf("SendFile result = %u\n", w);
	if( w >= 0 ) {
		printf("Starting file transfer\n");
		w = srv_TCPStartFileTransfer(FILET_PORT, filepath, filesize, &g_SendFileThread, &g_SendFileSocket);
	}
	free(result);
	return w;
}

int getFile(char* path) {
	PPACKET packet = protocol_BuildGetFilePacket(0xdeadbeef);
	if(!packet) 	return -1;
	int w = srv_WritePacket(g_sockfd, packet, sizeof(PACKET), key);
	free(packet);
	if( w < 0 ) 	return -1;
	PPACKET result = srv_ReadPacket(g_sockfd, MAX_PACKET, key);
	if( result == 0 )  	return -1;
	w = protocol_ParseGFileResultPacket(result);
	if( w >= 0 ) {
		w = srv_TCPFileTransfer(result->u.gfileres.ipaddr, result->u.gfileres.port, g_FileSz, path);
		if( w < 0 ) {
			packet = protocol_BuildFopDonePacket(-1);
		} else {
			packet = protocol_BuildFopDonePacket(0);
		}
		w = srv_WritePacket(g_sockfd, packet, sizeof(PACKET), key);
		free(packet);
	}
	free(result);
	return w;
}

int getAvatar(uint32_t uid, uint32_t* avatar_id) {
	PPACKET packet = protocol_BuildGetAvatarPacket(uid);
	if(!packet) 	return -1;
	int w = srv_WritePacket(g_sockfd, packet, sizeof(PACKET), key);
	free(packet);
	if( w < 0 ) 	return -1;
	PPACKET result = srv_ReadPacket(g_sockfd, MAX_PACKET, key);
	if( result == NULL ) 	return -1;
	w = protocol_ParseGAResultPacket(result, avatar_id);
	free(result);
	return w;
}

int setAvatar(uint32_t avatar_id) {
	PPACKET packet = protocol_BuildSetAvatarPacket(avatar_id);
        if(!packet)     return -1;
        int w = srv_WritePacket(g_sockfd, packet, sizeof(PACKET), key);
        free(packet);
        if( w < 0 )     return -1;
        PPACKET result = srv_ReadPacket(g_sockfd, MAX_PACKET, key);
        if( result == NULL )    return -1;
	w = protocol_ParseSAResultPacket(result);
	free(result);
	return w;
}

int delFriend(int uid) {
	if(uid == g_uid) 	return -1;
	PPACKET packet = protocol_BuildDeleteFriendPacket(uid);
	if(!packet) 	return -1;
	int w = srv_WritePacket(g_sockfd, packet, sizeof(PACKET), key);
	free(packet);
	if( w < 0 ) 	return -1;
	PPACKET result = srv_ReadPacket(g_sockfd, MAX_PACKET, key);
	if( result == NULL ) 	return -1;
	w = protocol_ParseDFResultPacket(result);
	free(result);
	return w;
}
