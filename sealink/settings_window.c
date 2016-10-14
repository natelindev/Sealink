//
//  settings_window.c
//  SeaLink
//
//  Created by 林理露 on 16/8/28.
//  Copyright © 2016年 林理露. All rights reserved.
//
#include <gtk/gtk.h>
#include "callbacks.h"
#include "resize_image.h"
#include "settings_window.h"
#include "global_settings.h"
#include "logger.h"

GtkWidget *create_ps_info_set(GtkWidget *window,set_info_widgets *my_set_info);
GtkWidget *create_gp_msg_set(GtkWidget *window,set_info_widgets *my_set_info);
GtkWidget *create_sys_set(GtkWidget *window,set_info_widgets *my_set_info);
GtkWidget *create_friend_set(GtkWidget *window,set_info_widgets *my_set_info);

GtkWidget *create_settings_window()
{
    GtkWidget *mainBox;
    GtkWidget *notebook;
    
    GtkWidget *ps_info_set_label;
    GtkWidget *gp_msg_set_label;
    GtkWidget *sys_set_label;
    GtkWidget *friend_set_label;
    
    GtkWidget *ps_info_set;
    GtkWidget *gp_msg_set;
    GtkWidget *sys_set;
    GtkWidget *friend_set;
    
    GtkWidget *bbox;
    GtkWidget *frame;
    GtkWidget *set_save_button;
    GtkWidget *set_cancel_button;
    
    set_info_widgets *my_set_info = NULL;
    my_set_info = g_malloc(sizeof(set_info_widgets));
    if (my_set_info == NULL) {
        logEvent(LOG_ERROR, "pointer [my_set_info] malloc FAILED.\n");
    }else{
        logEvent(LOG_DEBUG, "pointer [my_set_info] MALLOCED, size: %d.\n",sizeof(set_info_widgets));
    }
    
    my_set_info->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(my_set_info->window), 300, 150);
    gtk_window_set_position(GTK_WINDOW(my_set_info->window), GTK_WIN_POS_CENTER);
    gtk_window_set_title(GTK_WINDOW(my_set_info->window), "Sealink 设置");
    
    ps_info_set_label = gtk_label_new("个人资料设置");
    gp_msg_set_label = gtk_label_new("群设置");
    sys_set_label = gtk_label_new("系统设置");
    friend_set_label = gtk_label_new("好友设置");
    
    bbox = gtk_hbutton_box_new();
    frame = gtk_frame_new("");
    set_save_button = gtk_button_new_with_label("保存");
    set_cancel_button = gtk_button_new_with_label("取消");
    g_signal_connect(G_OBJECT(set_save_button), "clicked", G_CALLBACK(on_set_save_btn_clicked), my_set_info);
    g_signal_connect(G_OBJECT(set_cancel_button), "clicked", G_CALLBACK(on_set_cancel_btn_clicked), my_set_info->window);
    
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_SPREAD);
    gtk_container_add(GTK_CONTAINER(frame), bbox);
    gtk_box_pack_start(GTK_BOX(bbox), set_save_button, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(bbox), set_cancel_button, 0, 0, 5);
    
    mainBox = gtk_vbox_new(FALSE, 0);
    notebook = gtk_notebook_new();
    
    ps_info_set = create_ps_info_set(my_set_info->window,my_set_info);
    gp_msg_set = create_gp_msg_set(my_set_info->window,my_set_info);
    sys_set = create_sys_set(my_set_info->window,my_set_info);
    friend_set = create_friend_set(my_set_info->window,my_set_info);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),ps_info_set,ps_info_set_label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),gp_msg_set,gp_msg_set_label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),sys_set, sys_set_label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), friend_set, friend_set_label);
    
    GTK_NOTEBOOK(notebook)->tab_pos =GTK_POS_LEFT;
    gtk_box_pack_start(GTK_BOX(mainBox), notebook, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(mainBox), frame, 0, 0, 5);
    gtk_container_set_border_width(GTK_CONTAINER(bbox), 5);
    gtk_container_set_border_width(GTK_CONTAINER(mainBox), 20);
    
    gtk_container_add(GTK_CONTAINER(my_set_info->window), mainBox);
    gtk_widget_show_all(my_set_info->window);
    return my_set_info->window;
}

//个人资料设置
GtkWidget *create_ps_info_set(GtkWidget *window,set_info_widgets *my_set_info)
{
    GtkWidget *mainBox;
    GtkWidget *image;
    GtkWidget *avatar_set_label;
    GtkWidget *nickname_set_label;
    GtkWidget *tempBox;
    
    mainBox = gtk_vbox_new(FALSE, 0);
    
    my_set_info->avatar_button = gtk_button_new();
    gchar avatar_path[dest_len];
    sprintf(avatar_path, "%s/avatars/%d.jpg",g_s.main_path,g_s.avatar_id);
    image = gtk_image_new_from_file(avatar_path);
    resize_image(image, 64, 64);
    logEvent(LOG_INFO, "Reading avatar file from path: %s.\n",avatar_path);
    gtk_container_add(GTK_CONTAINER(my_set_info->avatar_button), image);
    g_signal_connect(G_OBJECT(my_set_info->avatar_button),"clicked",G_CALLBACK(on_set_avatar_btn_clicked),window);
    
    gchar *nickname = g_s.nick_name;
    avatar_set_label = gtk_label_new("当前头像:");
    nickname_set_label = gtk_label_new("昵称:");
    my_set_info->nickname_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(my_set_info->nickname_entry),nickname);
    
    tempBox = gtk_hbox_new(TRUE, 0);
    gtk_box_pack_start(GTK_BOX(tempBox), avatar_set_label, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(tempBox), my_set_info->avatar_button, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(mainBox), tempBox, 0, 0, 5);
    
    tempBox = gtk_hbox_new(TRUE, 0);
    gtk_box_pack_start(GTK_BOX(tempBox), nickname_set_label, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(tempBox), my_set_info->nickname_entry, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(mainBox), tempBox, 0, 0, 5);
    
    gtk_container_set_border_width(GTK_CONTAINER(mainBox), 10);
    return mainBox;
}

//群消息设置
GtkWidget *create_gp_msg_set(GtkWidget *window,set_info_widgets *my_set_info)
{
    GtkWidget *mainBox;
    GtkWidget *tempBox;
    GtkWidget *notify_method_label;
    GtkWidget *add_group_entry;
    GtkWidget *add_group_label;
    GtkWidget *add_group_button;
    
    mainBox = gtk_vbox_new(FALSE, 0);
    my_set_info->combo_box = gtk_combo_box_new_text();
    notify_method_label = gtk_label_new("选择群消息提醒方式:");
    gtk_combo_box_append_text(GTK_COMBO_BOX(my_set_info->combo_box), "接收并提醒群消息");
    gtk_combo_box_append_text(GTK_COMBO_BOX(my_set_info->combo_box), "接收但不提醒群消息");
    gtk_combo_box_append_text(GTK_COMBO_BOX(my_set_info->combo_box), "完全屏蔽群消息");
    gtk_combo_box_set_active(GTK_COMBO_BOX(my_set_info->combo_box), g_s.notify_method);
    
    tempBox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(tempBox), notify_method_label, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(tempBox), my_set_info->combo_box, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(mainBox), tempBox, 0, 0, 5);

    tempBox = gtk_hbox_new(FALSE, 5);
    add_group_entry = gtk_entry_new();
    add_group_label = gtk_label_new("请输入群ID:");
    add_group_button = gtk_button_new_with_label("添加群");
    
    gtk_box_pack_start(GTK_BOX(tempBox), add_group_label, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(tempBox), add_group_entry, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(tempBox), add_group_button, 0, 0, 5);
    g_signal_connect(G_OBJECT(add_group_button), "clicked", G_CALLBACK(on_add_group_btn_clicked), add_group_entry);
    
    gtk_box_pack_start(GTK_BOX(mainBox), tempBox, 0, 0, 5);
    gtk_container_set_border_width(GTK_CONTAINER(mainBox), 10);
    return mainBox;
}

//系统设置
GtkWidget *create_sys_set(GtkWidget *window,set_info_widgets *my_set_info)
{
    GtkWidget *mainBox;
    GtkWidget *tempBox;
    GtkWidget *file_save_path_label;

    mainBox = gtk_vbox_new(FALSE, 0);
    file_save_path_label = gtk_label_new("当前文件存储路径:");
    my_set_info->file_save_path_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(my_set_info->file_save_path_entry), g_s.main_path);
    
    tempBox = gtk_hbox_new(FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(tempBox), file_save_path_label, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(tempBox), my_set_info->file_save_path_entry, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(mainBox), tempBox, 0, 0, 5);
    
    gtk_container_set_border_width(GTK_CONTAINER(mainBox), 10);
    return mainBox;
    
}

//好友设置
GtkWidget *create_friend_set(GtkWidget *window,set_info_widgets *my_set_info){
    GtkWidget *mainBox;
    GtkWidget *add_entry;
    GtkWidget *add_label;
    GtkWidget *add_friend_button;
    GtkWidget *del_label;
    GtkWidget *del_entry;
    GtkWidget *del_friend_button;
    GtkWidget *tempBox;
    
    mainBox = gtk_vbox_new(FALSE, 0);
    add_entry = gtk_entry_new();
    add_label = gtk_label_new("对方的ID:");
    add_friend_button = gtk_button_new_with_label("添加好友");
    del_label = gtk_label_new("对方的ID:");
    del_entry = gtk_entry_new();
    del_friend_button = gtk_button_new_with_label("删除好友");
    
    tempBox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(tempBox), add_label, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(tempBox), add_entry, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(tempBox), add_friend_button, 0, 0, 5);
    g_signal_connect(G_OBJECT(add_friend_button),"clicked",G_CALLBACK(on_add_friend_btn_clicked),add_entry);
    gtk_box_pack_start(GTK_BOX(mainBox), tempBox, 0, 0, 5);
    
    tempBox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(tempBox), del_label, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(tempBox), del_entry, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(tempBox), del_friend_button, 0, 0, 5);
    g_signal_connect(G_OBJECT(del_friend_button),"clicked",G_CALLBACK(on_del_friend_btn_clicked),del_entry);
    
    gtk_box_pack_start(GTK_BOX(mainBox), tempBox, 0, 0, 5);
    
    gtk_container_set_border_width(GTK_CONTAINER(mainBox), 10);
    
    return mainBox;
}

