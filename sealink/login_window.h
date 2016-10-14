//
//  login_window.h
//  SeaLink
//
//  Created by 林理露 on 16/8/28.
//  Copyright © 2016年 林理露. All rights reserved.
//

#ifndef login_window_h
#define login_window_h

GtkWidget *create_login_window();

typedef struct{
    GtkWidget *window;
    GtkWidget *username_entry;
    GtkWidget *password_entry;
}user_login_info;

#endif /* login_window_h */
