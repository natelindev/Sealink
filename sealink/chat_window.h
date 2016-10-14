//
//  chat_window.h
//  SeaLink
//
//  Created by 林理露 on 16/8/25.
//  Copyright © 2016年 林理露. All rights reserved.
//

#ifndef chat_window_h
#define chat_window_h
#include "user_dao.h"
#include "group_dao.h"
GtkWidget *create_chat_window(simple_user_info* u_info);

typedef struct{
    GdkColor text_color;
    int text_size;
}text_style_info;

typedef struct {
	simple_user_info* u_info;
	GtkWidget* send_text_view;
	GtkWidget* receive_text_view;
} msg_info;

typedef struct{
    group_info *g_info;
    GtkWidget* send_text_view;
    GtkWidget* receive_text_view;
}group_msg_info;
#endif /* chat_window_h */
