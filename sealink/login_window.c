//
//  login_window.c
//  SeaLink
//
//  Created by 林理露 on 16/8/28.
//  Copyright © 2016年 林理露. All rights reserved.
//

#include <gtk/gtk.h>
#include "callbacks.h"
#include "logger.h"
#include "login_window.h"

GtkWidget *create_login_window(){
    
    GtkWidget* mainBox;
    GtkWidget* bbox;
    GtkWidget* login_button;
    GtkWidget* login_label;
    GtkWidget* register_button;
    GtkWidget* register_label;
    user_login_info *my_login_info = NULL;
    
    my_login_info = g_malloc(sizeof(user_login_info));
    if (my_login_info == NULL) {
        logEvent(LOG_ERROR, "pointer [my_login_info] malloc FAILED.\n");
    }else{
        logEvent(LOG_DEBUG, "pointer [my_login_info] MALLOCED, size: %d.\n",sizeof(user_login_info));
    }

    my_login_info->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(my_login_info->window),"SeaLink 登录界面");
    gtk_window_set_position(GTK_WINDOW(my_login_info->window),GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(my_login_info->window),200,150);
    
    mainBox = gtk_vbox_new(FALSE, 0);
    bbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_SPREAD);
    my_login_info->username_entry = gtk_entry_new();
    my_login_info->password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(my_login_info->password_entry), FALSE);
    login_button = gtk_button_new_with_label("登录");
    register_button = gtk_button_new_with_label("注册");
    
    login_label = gtk_label_new("用户名:");
    register_label = gtk_label_new("密码:");
    g_signal_connect(G_OBJECT(login_button), "clicked", G_CALLBACK(on_login_btn_clicked), my_login_info);
    g_signal_connect(G_OBJECT(register_button), "clicked", G_CALLBACK(on_register_btn_clicked), NULL);
    
    gtk_box_pack_start(GTK_BOX(bbox), login_button, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(bbox), register_button, 0, 0, 5);
    
    gtk_box_pack_start(GTK_BOX(mainBox), login_label, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(mainBox), my_login_info->username_entry, 1, 1, 0);
    gtk_box_pack_start(GTK_BOX(mainBox), register_label, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(mainBox), my_login_info->password_entry, 1, 1, 0);
    gtk_box_pack_start(GTK_BOX(mainBox), bbox, 0, 0, 10);
    
    gtk_container_add(GTK_CONTAINER(my_login_info->window), mainBox);
    gtk_container_set_border_width(GTK_CONTAINER(mainBox), 20);
    
    gtk_widget_show_all(my_login_info->window);
    g_signal_connect(G_OBJECT(my_login_info->window),"delete_event",G_CALLBACK(on_login_window_delete),my_login_info);
    return my_login_info->window;
}

