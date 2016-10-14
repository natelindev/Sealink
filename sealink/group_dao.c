//
//  group_dao.c
//  sealink
//
//  Created by 林理露 on 16/8/31.
//  Copyright © 2016年 林理露. All rights reserved.
//

#include <gtk/gtk.h>
#include "group_dao.h"
#include "global_settings.h"

#include "common.h"

extern uint32_t g_uid;

GList *get_group_list(){
	/*
    group_info *groups = g_malloc(6*sizeof(group_info));
    GList *group_list = NULL;

    gint i = 0;
    
    for (i = 0; i < 6; ++i) {
        groups[i].GID = i+1;
    }
    
    for (i = 0 ; i < 6; ++i) {
        group_list = g_list_append(group_list, &groups[i]);
    }
    */
	int groupcount, *groups_i;
    int result = queryGroup(g_uid, &groupcount, &groups_i);
    if(result < 0) 	return NULL;
    group_info *groups = g_malloc(groupcount*sizeof(group_info));
    GList *group_list = NULL;

    gint i =0;
    for(i=0;i<groupcount;i++) {
	    groups[i].GID = groups_i[i];
    }
    for(i=0;i<groupcount;i++) {
	    group_list = g_list_append(group_list, &groups[i]);
    }
    return group_list;
}
