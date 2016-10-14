//
//  register_window.c
//  sealink
//
//  Created by 林理露 on 16/8/28.
//  Copyright © 2016年 林理露. All rights reserved.
//

#include <gtk/gtk.h>
#include "callbacks.h"
#include "logger.h"
#include "register_window.h"

GtkWidget *create_register_window(){
    
    GtkWidget *mainBox;
    GtkWidget *username_label;
    GtkWidget *password_label;
    GtkWidget *password_confirm_label;
    GtkWidget *bbox;
    GtkWidget *confirm_button;
    GtkWidget *cancel_button;
    user_reg_info *my_reg_info;
    
    my_reg_info = g_malloc(sizeof(user_reg_info));
    logEvent(LOG_DEBUG, "pointer [my_reg_info] MALLOCED, size: %d\n",sizeof(my_reg_info));
    my_reg_info->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(my_reg_info->window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(my_reg_info->window), 300, 250);
    gtk_window_set_title(GTK_WINDOW(my_reg_info->window), "Sealink 新用户注册");
    
    mainBox = gtk_vbox_new(FALSE, 0);
    username_label = gtk_label_new("请输入你的用户名:");
    password_label = gtk_label_new("请输入你的密码:");
    password_confirm_label = gtk_label_new("请再次输入你的密码:");
    my_reg_info->username_entry = gtk_entry_new();
    my_reg_info->password_entry = gtk_entry_new();
    my_reg_info->password_confirm_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(my_reg_info->password_entry), FALSE);
    gtk_entry_set_visibility(GTK_ENTRY(my_reg_info->password_confirm_entry), FALSE);
    bbox = gtk_hbutton_box_new();
    confirm_button = gtk_button_new_with_label("确认");
    cancel_button = gtk_button_new_with_label("取消");
    
    g_signal_connect(G_OBJECT(confirm_button), "clicked", G_CALLBACK(on_reg_confirm_btn_clicked), my_reg_info);
    g_signal_connect(G_OBJECT(cancel_button), "clicked", G_CALLBACK(on_reg_cancel_btn_clicked), my_reg_info);
    
    gtk_box_pack_start(GTK_BOX(bbox), confirm_button, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(bbox), cancel_button, 0, 0, 5);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_SPREAD);
    
    gtk_box_pack_start(GTK_BOX(mainBox),username_label, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(mainBox),my_reg_info->username_entry, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(mainBox),password_label, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(mainBox),my_reg_info->password_entry, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(mainBox),password_confirm_label, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(mainBox),my_reg_info->password_confirm_entry, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(mainBox), bbox, 0, 0, 5);
    
    gtk_container_set_border_width(GTK_CONTAINER(mainBox), 20);
    gtk_container_add(GTK_CONTAINER(my_reg_info->window), mainBox);
    gtk_widget_show_all(my_reg_info->window);
    g_signal_connect(G_OBJECT(my_reg_info->window),"delete_event",G_CALLBACK(on_reg_window_delete),my_reg_info);
    return my_reg_info->window;
}
