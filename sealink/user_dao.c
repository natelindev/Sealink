//
//  user_dao.c
//  sealink
//
//  Created by 林理露 on 16/8/31.
//  Copyright © 2016年 林理露. All rights reserved.
//
#include <gtk/gtk.h>
#include "global_settings.h"
#include "user_dao.h"

#include "common.h"

extern int g_sockfd;
extern uint8_t* key;

int QueryUserName(int uid, char* buffer) {
	PPACKET packet = protocol_BuildGetNamePacket(uid);
	if(!packet) {
		return -1;
	}
	int w = srv_WritePacket(g_sockfd, packet, sizeof(PACKET), key);
	free(packet);
	if( w < 0 ) 	return -1;
	PPACKET returned = srv_ReadPacket(g_sockfd, MAX_PACKET, key);
	uint32_t result = protocol_ParseGNResultPacket(returned);
	if( result == 0 )
		strncpy(buffer, returned->u.gnres.username, 32);
	return result;
}

GList* get_user_list(){
	/*
    GList *user_list = NULL;
    simple_user_info *users = g_malloc(6*sizeof(simple_user_info));
    
    gint i = 0;
    
    for (i = 0; i < 6; ++i) {
        users[i].avatar_id = i+1;
        sprintf(users[i].uname, "北理第%d提莫",i+2);
    }
    
    for (i = 0 ; i < 6; ++i) {
        user_list = g_list_append(user_list, &users[i]);
    }
    
    return user_list;*/
	PPACKET packet = protocol_BuildQueryFriendPacket(0xdeadbeef);
	if(!packet) {
		return NULL;
	}
	int w = srv_WritePacket(g_sockfd, packet, sizeof(PACKET), key);
	free(packet);
	uint32_t result, fcount;
	if( w >= 0 ) {
		uint8_t* buffer = srv_ReadPacket(g_sockfd, MAX_PACKET, key);
		if( !buffer ) {
			return NULL;
		} else {
			result = protocol_ParseQFResultPacket(buffer, &fcount);
			free(buffer);
		}
	}
	uint32_t* flist = srv_ReadPacket(g_sockfd, fcount * sizeof(uint32_t), key);
	if( !flist ) {
		return NULL;
	}
	simple_user_info *users = g_malloc(fcount * sizeof(simple_user_info));
	gint i = 0;
	for(i=0; i<fcount; i++) {
		uint32_t aid;
		getAvatar(flist[i], &aid);
		users[i].avatar_id = aid;
		users[i].UID = flist[i];
		QueryUserName(flist[i], users[i].uname);
	}

	GList *glist = NULL;
	for(i=0;i<fcount;i++) {
		glist = g_list_append(glist, &users[i]);
	}

	return glist;
}
