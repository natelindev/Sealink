//
//  register_window.h
//  sealink
//
//  Created by 林理露 on 16/8/28.
//  Copyright © 2016年 林理露. All rights reserved.
//

#ifndef register_window_h
#define register_window_h

GtkWidget *create_register_window();

typedef struct{
    GtkWidget *window;
    GtkWidget *username_entry;
    GtkWidget *password_entry;
    GtkWidget *password_confirm_entry;
}user_reg_info;

#endif /* register_window_h */
