//
//  chat_window.c
//  SeaLink
//
//  Created by 林理露 on 16/8/25.
//  Copyright © 2016年 林理露. All rights reserved.
//
#include <gtk/gtk.h>
#include "chat_window.h"
#include "callbacks.h"
#include "logger.h"
#include "user_dao.h"
#include "global_settings.h"
#include "resize_image.h"

GtkWidget *create_chat_headbar(GtkWidget *window,simple_user_info *u_info);
GtkWidget *create_group_headbar(GtkWidget *window,group_info *g_info);
GtkWidget *create_chat_toolbar(GtkWidget *window,GtkWidget *text_view);
GtkWidget *create_chat_bottombar(GtkWidget *window,msg_info* m_i);
GtkWidget *create_group_bottombar(GtkWidget *window,group_msg_info* g_m_i);
GtkWidget *create_sidebar(GtkWidget*window,simple_user_info *u_info);
void on_chat_window_destroy(GtkWidget* widget, GdkEvent event, int UID);
void on_group_window_destroy(GtkWidget* widget, GdkEvent event, int GID);

extern unsigned int g_uid;

GtkWidget *create_chat_window(simple_user_info *u_info)
{
    //查重
    gboolean exists = FALSE;
    GList *temp_win = windows;
    while (temp_win != NULL) {
        if (((chat_info*)temp_win->data)->UID == u_info->UID) {
            logEvent(LOG_INFO, "Duplicate window found.\n");
            exists = TRUE;
            break;
        }
        temp_win = g_list_next(temp_win);
    }
    
    if (!exists) {
        GtkWidget *window;
        GtkWidget *mainBox;
        GtkWidget *leftBox;
        GtkWidget *rightBox;
        GtkWidget *separator;
        GtkWidget *headbar;
        
        GtkWidget *send_window;
        GtkWidget* receive_window;
        
        GtkWidget *toolbar;
        GtkWidget *bottombar;
        
        GtkWidget *file_rec_button;
        
        msg_info *m_i;
        chat_info *c_i;
        
        c_i = g_malloc(sizeof(chat_info));
        m_i = g_malloc(sizeof(msg_info));
        m_i->u_info = u_info;
        c_i->UID = m_i->u_info->UID;
        
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), u_info->uname);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
        gtk_window_set_default_size(GTK_WINDOW(window), 600, 450);
        
        mainBox = gtk_hbox_new(FALSE, 0);
        leftBox = gtk_vbox_new(FALSE, 0);
        rightBox = create_sidebar(window,u_info);
        separator = gtk_vseparator_new();
        gtk_box_pack_start(GTK_BOX(mainBox), leftBox, 0, 0, 5);
        gtk_box_pack_start(GTK_BOX(mainBox), separator, 0, 0, 5);
        gtk_box_pack_start(GTK_BOX(mainBox), rightBox, 0, 0, 5);
        
        headbar = create_chat_headbar(window,u_info);
        gtk_box_pack_start(GTK_BOX(leftBox), headbar, 0, 0, 5);
        
        m_i->send_text_view = gtk_text_view_new();
        m_i->receive_text_view = gtk_text_view_new();
        c_i->receive_text = m_i->receive_text_view;
        
        send_window = gtk_scrolled_window_new(NULL, NULL);
        GTK_SCROLLED_WINDOW(send_window)->hscrollbar_policy = GTK_POLICY_AUTOMATIC;
        GTK_SCROLLED_WINDOW(send_window)->vscrollbar_policy = GTK_POLICY_AUTOMATIC;
        gtk_container_add(GTK_CONTAINER(send_window), m_i->send_text_view);
        
        
        receive_window = gtk_scrolled_window_new(NULL, NULL);
        gtk_text_view_set_editable(GTK_TEXT_VIEW(m_i->receive_text_view), FALSE);
        gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(m_i->receive_text_view), FALSE);
        GTK_SCROLLED_WINDOW(receive_window)->hscrollbar_policy = GTK_POLICY_AUTOMATIC;
        GTK_SCROLLED_WINDOW(receive_window)->vscrollbar_policy = GTK_POLICY_AUTOMATIC;
        gtk_container_add(GTK_CONTAINER (receive_window), m_i->receive_text_view);
        
        
        gtk_box_pack_start(GTK_BOX(leftBox), receive_window, 1, 1, 5);
        
        toolbar = create_chat_toolbar(window,m_i->send_text_view);
        gtk_box_pack_start(GTK_BOX(leftBox), toolbar, 0, 0, 5);
        
        gtk_box_pack_start(GTK_BOX(leftBox), send_window, 1, 1, 5);
        
        bottombar = create_chat_bottombar(window,m_i);
        gtk_box_pack_end(GTK_BOX(leftBox), bottombar, 0, 0, 5);
        
        
        
        gtk_container_add(GTK_CONTAINER(window), mainBox);
        gtk_container_set_border_width(GTK_CONTAINER(mainBox),20);
        
        gtk_widget_show_all(window);
        g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(on_chat_window_destroy), u_info->UID);
        windows = g_list_append(windows, c_i);
        return window;
    }else{
        return NULL;
    }
}

GtkWidget *create_group_chat_window(group_info *g_info){
    //查重
    gboolean exists = FALSE;
    GList *temp_win = group_windows;
    while (temp_win != NULL) {
        if (((group_info*)temp_win->data)->GID == g_info->GID) {
            logEvent(LOG_INFO, "Duplicate group window found.\n");
            exists = TRUE;
            break;
        }
        temp_win = g_list_next(temp_win);
    }
    
    if (!exists) {
        GtkWidget *window;
        GtkWidget *mainBox;
        GtkWidget *leftBox;
        GtkWidget *rightBox;
        GtkWidget *separator;
        GtkWidget *headbar;
        
        GtkWidget *send_window;
        GtkWidget* receive_window;
        
        GtkWidget *toolbar;
        GtkWidget *bottombar;
        gchar name[500];
        
        group_msg_info *g_m_i;
        chat_info *c_i;
        
        c_i = g_malloc(sizeof(chat_info));
        g_m_i = g_malloc(sizeof(group_msg_info));
        g_m_i->g_info = g_info;
        c_i->UID = g_m_i->g_info->GID;
        
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        sprintf(name, "群组 %d",g_info->GID);
        gtk_window_set_title(GTK_WINDOW(window), name);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
        gtk_window_set_default_size(GTK_WINDOW(window), 600, 450);
        
        mainBox = gtk_hbox_new(FALSE, 0);
        leftBox = gtk_vbox_new(FALSE, 0);
        gtk_widget_set_usize(leftBox, 350, 450);
        rightBox = gtk_vbox_new(FALSE, 0);
        separator = gtk_vseparator_new();
        gtk_box_pack_start(GTK_BOX(mainBox), leftBox, 0, 0, 5);
        gtk_box_pack_start(GTK_BOX(mainBox), separator, 0, 0, 5);
        gtk_box_pack_start(GTK_BOX(mainBox), rightBox, 0, 0, 5);
        
        headbar = create_group_headbar(window,g_info);
        gtk_box_pack_start(GTK_BOX(leftBox), headbar, 0, 0, 5);
        
        g_m_i->send_text_view = gtk_text_view_new();
        g_m_i->receive_text_view = gtk_text_view_new();
        c_i->receive_text = g_m_i->receive_text_view;
        
        send_window = gtk_scrolled_window_new(NULL, NULL);
        GTK_SCROLLED_WINDOW(send_window)->hscrollbar_policy = GTK_POLICY_AUTOMATIC;
        GTK_SCROLLED_WINDOW(send_window)->vscrollbar_policy = GTK_POLICY_AUTOMATIC;
        gtk_container_add(GTK_CONTAINER(send_window), g_m_i->send_text_view);
        
        
        receive_window = gtk_scrolled_window_new(NULL, NULL);
        gtk_text_view_set_editable(GTK_TEXT_VIEW(g_m_i->receive_text_view), FALSE);
        gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(g_m_i->receive_text_view), FALSE);
        GTK_SCROLLED_WINDOW(receive_window)->hscrollbar_policy = GTK_POLICY_AUTOMATIC;
        GTK_SCROLLED_WINDOW(receive_window)->vscrollbar_policy = GTK_POLICY_AUTOMATIC;
        gtk_container_add(GTK_CONTAINER (receive_window), g_m_i->receive_text_view);
        
        
        gtk_box_pack_start(GTK_BOX(leftBox), receive_window, 1, 1, 5);
        
        toolbar = create_chat_toolbar(window,g_m_i->send_text_view);
        gtk_box_pack_start(GTK_BOX(leftBox), toolbar, 0, 0, 5);
        
        gtk_box_pack_start(GTK_BOX(leftBox), send_window, 1, 1, 5);
        
        bottombar = create_group_bottombar(window,g_m_i);
        gtk_box_pack_end(GTK_BOX(leftBox), bottombar, 0, 0, 5);
        
        gtk_container_add(GTK_CONTAINER(window), mainBox);
        gtk_container_set_border_width(GTK_CONTAINER(mainBox),20);
        
        gtk_widget_show_all(window);
        g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(on_group_window_destroy), g_info->GID);
        group_windows = g_list_append(group_windows, c_i);
        return window;
    }else{
        return NULL;
    }

}

GtkWidget *create_group_headbar(GtkWidget *window,group_info *g_info){
    GtkWidget *mainBox;
    GtkWidget *bbox;

    GtkWidget *chat_history_button;
    GtkWidget *detail_info_button;
    GtkWidget *target_label;
    
    mainBox = gtk_vbox_new(FALSE, 0);
    bbox = gtk_hbutton_box_new();
    
    gchar name[dest_len];
    sprintf(name, "群组 %d",g_info->GID);
    target_label = gtk_label_new(name);
    
    gtk_box_pack_start(GTK_BOX(mainBox), target_label, 0, 0, 10);
    gtk_box_pack_start(GTK_BOX(mainBox), bbox, 0, 0, 2);
    
    chat_history_button = gtk_button_new_with_label("聊天记录");
    gtk_box_pack_start(GTK_BOX(bbox), chat_history_button, 0, 0, 2);
    g_signal_connect(G_OBJECT(chat_history_button),"clicked",G_CALLBACK(on_chat_history_btn_clicked),NULL);
    
    detail_info_button = gtk_button_new_with_label("群组资料");
    gtk_box_pack_start(GTK_BOX(bbox), detail_info_button, 0, 0, 2);
    g_signal_connect(G_OBJECT(detail_info_button),"clicked",G_CALLBACK(on_detail_info_btn_clicked),NULL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_SPREAD);
    return mainBox;
}


GtkWidget *create_group_bottombar(GtkWidget *window,group_msg_info* g_m_i){
    GtkWidget *bottombar = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bottombar),GTK_BUTTONBOX_SPREAD);
    GtkWidget *send_button;
    GtkWidget *close_button;
    
    send_button = gtk_button_new_with_label("发送");
    close_button = gtk_button_new_with_label("关闭");
    gtk_box_pack_start(GTK_BOX(bottombar), send_button, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(bottombar), close_button,0, 0, 5);
    g_signal_connect(G_OBJECT(send_button), "clicked", G_CALLBACK(on_group_chat_send_btn_clicked),g_m_i);
    g_signal_connect(G_OBJECT(close_button), "clicked", G_CALLBACK(on_chat_close_btn_clicked), window);
    return bottombar;
}

void on_group_window_destroy(GtkWidget* widget, GdkEvent event, int GID){
    GList *temp_win = group_windows;
    while (temp_win != NULL) {
        if (((group_info*)temp_win->data)->GID == GID) {
            logEvent(LOG_INFO, "Group window to destroy found.\n");
            group_windows = g_list_remove(group_windows, temp_win->data);
            break;
        }
        temp_win = g_list_next(temp_win);
    }
}

void on_chat_window_destroy(GtkWidget* widget, GdkEvent event, int UID){
    GList *temp_win = windows;
    while (temp_win != NULL) {
        if (((chat_info*)temp_win->data)->UID == UID) {
            logEvent(LOG_INFO, "Window to destroy found.\n");
            windows = g_list_remove(windows, temp_win->data);
            break;
        }
        temp_win = g_list_next(temp_win);
    }
}

GtkWidget *create_chat_headbar(GtkWidget *window,simple_user_info *u_info){
    GtkWidget *mainBox;
    GtkWidget *image;
    GtkWidget *top_LeftBox;
    GtkWidget *top_RightBox;
    GtkWidget *bbox;
    GtkWidget *file_transfer_button;
    GtkWidget *file_rec_button;
    GtkWidget *skin_change_button;
    GtkWidget *target_label;
    
    mainBox = gtk_hbox_new(FALSE, 0);
    top_LeftBox = gtk_vbox_new(FALSE, 0);
    top_RightBox = gtk_vbox_new(FALSE, 0);
    bbox = gtk_hbutton_box_new();
    target_label = gtk_label_new(u_info->uname);
    gchar path[dest_len];
    sprintf(path, "%s/avatars/%d.jpg",g_s.main_path,u_info->avatar_id);
    image = gtk_image_new_from_file(path);
    resize_image(image, 64, 64);
    
    gtk_box_pack_start(GTK_BOX(top_LeftBox), image, 0, 0, 0);
    
    gtk_box_pack_start(GTK_BOX(mainBox), top_LeftBox, 0, 0, 15);
    gtk_box_pack_start(GTK_BOX(mainBox), top_RightBox, 0, 0, 2);
    
    gtk_box_pack_start(GTK_BOX(top_RightBox), target_label, 0, 0, 10);
    gtk_box_pack_start(GTK_BOX(top_RightBox), bbox, 0, 0, 2);
    
    chat_info *c_i = g_malloc(sizeof(chat_info));
    c_i->UID = u_info->UID;
    c_i->receive_text = window;
    
    file_transfer_button = gtk_button_new_with_label("文件传输");
    gtk_box_pack_start(GTK_BOX(bbox), file_transfer_button, 0, 0, 2);
    g_signal_connect(G_OBJECT(file_transfer_button),"clicked",G_CALLBACK(on_file_tranfer_btn_clicked),c_i);
    
    
    file_rec_button = gtk_button_new_with_label("接收文件");
    gtk_box_pack_start(GTK_BOX(bbox), file_rec_button, 0, 0, 0);
    g_signal_connect(G_OBJECT(file_rec_button),"clicked",G_CALLBACK(on_file_rec_btn_clicked),window);
    gtk_box_pack_start(GTK_BOX(bbox), file_rec_button, 0, 0, 5);
    
    skin_change_button = gtk_button_new_with_label("更换皮肤");
    gtk_box_pack_start(GTK_BOX(bbox), skin_change_button, 0, 0, 0);
    g_signal_connect(G_OBJECT(skin_change_button),"clicked",G_CALLBACK(on_skin_change_btn_clicked),window);
    gtk_box_pack_start(GTK_BOX(bbox), skin_change_button, 0, 0, 5);
    
    return mainBox;
}


GtkWidget *create_chat_toolbar(GtkWidget *window,GtkWidget *text_view)
{
    GtkWidget *toolbar = gtk_toolbar_new();
    GTK_TOOLBAR(toolbar)->style_set = TRUE;
    GTK_TOOLBAR(toolbar)->style = GTK_TOOLBAR_ICONS;
    GTK_TOOLBAR(toolbar)->icon_size_set = TRUE;
    GTK_TOOLBAR(toolbar)->icon_size = 22;
    GtkToolItem *text_style_button;
    GtkToolItem *text_color_button;
    GtkToolItem *face_button;
    GtkToolItem *screenshot_button;
    GtkToolItem *send_image_button;
    GtkWidget *image;
    gchar path[dest_len];
    sprintf(path, "%s/icons/text_style.png",g_s.main_path);
    image = gtk_image_new_from_file(path);
    resize_image(image,24,24);
    text_style_button = gtk_tool_button_new(image, NULL);
    gtk_toolbar_insert (GTK_TOOLBAR(toolbar),text_style_button,0);
    gtk_tool_item_set_homogeneous(GTK_TOOL_ITEM(text_style_button), TRUE);
    
    g_signal_connect(G_OBJECT(text_style_button),"clicked",G_CALLBACK(on_text_style_btn_clicked),text_view);
    
    sprintf(path, "%s/icons/text_color.png",g_s.main_path);
    image = gtk_image_new_from_file(path);
    resize_image(image,24,24);
    text_color_button = gtk_tool_button_new(image, NULL);
    gtk_toolbar_insert (GTK_TOOLBAR(toolbar),text_color_button,1);
    gtk_tool_item_set_homogeneous(GTK_TOOL_ITEM(text_color_button), TRUE);
    
    g_signal_connect(G_OBJECT(text_color_button),"clicked",G_CALLBACK(on_text_color_btn_clicked),text_view);
    
    sprintf(path, "%s/icons/face.png",g_s.main_path);
    image = gtk_image_new_from_file(path);
    resize_image(image,24,24);
    face_button = gtk_tool_button_new(image, NULL);
    gtk_toolbar_insert (GTK_TOOLBAR(toolbar),face_button,2);
    gtk_tool_item_set_homogeneous(GTK_TOOL_ITEM(face_button), TRUE);
    
    g_signal_connect(G_OBJECT(face_button),"clicked",G_CALLBACK(on_face_btn_clicked),window);
    
    sprintf(path, "%s/icons/screenshot.png",g_s.main_path);
    image = gtk_image_new_from_file(path);
    resize_image(image,24,24);
    screenshot_button = gtk_tool_button_new(image, NULL);
    gtk_toolbar_insert (GTK_TOOLBAR(toolbar),screenshot_button,3);
    gtk_tool_item_set_homogeneous(GTK_TOOL_ITEM(screenshot_button), TRUE);
    g_signal_connect(G_OBJECT(screenshot_button),"clicked",G_CALLBACK(on_screen_shot_btn_clicked),NULL);
    
    sprintf(path, "%s/icons/send_image.png",g_s.main_path);
    image = gtk_image_new_from_file(path);
    resize_image(image,24,24);
    send_image_button = gtk_tool_button_new(image, NULL);
    gtk_tool_item_set_homogeneous(GTK_TOOL_ITEM(send_image_button), TRUE);
    
    gtk_toolbar_insert (GTK_TOOLBAR(toolbar),send_image_button,4);
    g_signal_connect(G_OBJECT(send_image_button),"clicked",G_CALLBACK(on_send_image_btn_clicked),window);
    return toolbar;
}

GtkWidget *create_chat_bottombar(GtkWidget *window,msg_info* m_i){
    GtkWidget *bottombar = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bottombar),GTK_BUTTONBOX_SPREAD);
    GtkWidget *send_button;
    GtkWidget *close_button;
    
    send_button = gtk_button_new_with_label("发送");
    close_button = gtk_button_new_with_label("关闭");
    gtk_box_pack_start(GTK_BOX(bottombar), send_button, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(bottombar), close_button,0, 0, 5);
    g_signal_connect(G_OBJECT(send_button), "clicked", G_CALLBACK(on_chat_send_btn_clicked),m_i);
    g_signal_connect(G_OBJECT(close_button), "clicked", G_CALLBACK(on_chat_close_btn_clicked), window);
    return bottombar;
}

GtkWidget *create_sidebar(GtkWidget*window,simple_user_info *u_info){
    GtkWidget *mainBox;
    GtkWidget *side_view;
    GtkWidget *sw;
    GtkWidget *notebook;
    GtkWidget *detail_info_label;
    GtkWidget *chat_history_label;
    GtkWidget *detail_info;
    GtkWidget *chat_history;
    
    GtkTextBuffer *buffer;
    
    mainBox = gtk_vbox_new(FALSE, 0);
    notebook = gtk_notebook_new();
    gchar text[1024];
    
    detail_info_label = gtk_label_new("详细资料");
    side_view = gtk_text_view_new();
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(side_view));
    sprintf(text, "好友名:\n%s\n UID: %d\n",u_info->uname,u_info->UID);
    gtk_text_buffer_set_text(buffer, text, (gint)strlen(text));
    
    detail_info = gtk_vbox_new(FALSE, 0);
    
    sw = gtk_scrolled_window_new(NULL, NULL);
    GTK_SCROLLED_WINDOW(sw)->hscrollbar_policy = GTK_POLICY_AUTOMATIC;
    GTK_SCROLLED_WINDOW(sw)->vscrollbar_policy = GTK_POLICY_AUTOMATIC;
    
    gtk_box_pack_start(GTK_BOX(detail_info_label), sw, 0, 0, 5);
    gtk_container_add(GTK_CONTAINER(sw), side_view);
    gtk_container_add(GTK_CONTAINER(detail_info), sw);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), detail_info, detail_info_label);
    
    chat_history_label = gtk_label_new("聊天记录");
    
    chat_history = gtk_vbox_new(FALSE, 0);
    sw = gtk_scrolled_window_new(NULL, NULL);
    GTK_SCROLLED_WINDOW(sw)->hscrollbar_policy = GTK_POLICY_AUTOMATIC;
    GTK_SCROLLED_WINDOW(sw)->vscrollbar_policy = GTK_POLICY_AUTOMATIC;
    
    side_view = gtk_text_view_new();
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(side_view));
     sprintf(text, "聊天记录: \n");
    gchar path[dest_len];
    sprintf(path, "%s/chat_history/%d_%d.chist",g_s.main_path,g_uid,u_info->UID);
    logEvent(LOG_INFO, "Chist path = %s\n", path);
    FILE *fp;
    fp = fopen(path, "r");
    if(fp != NULL ) {
	    while (!feof(fp)) {
		fread(text, sizeof(gchar), 1023, fp);
		text[1023] = 0;
		logEvent(LOG_INFO, "Chist = %s\n", text);
		appendText(GTK_TEXT_VIEW(side_view), text);
	    }
	}
    //gtk_text_buffer_set_text(buffer, text, (gint)strlen(text));

    gtk_container_add(GTK_CONTAINER(sw), side_view);
    gtk_box_pack_start(GTK_BOX(chat_history), sw, 1, 1, 5);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), chat_history, chat_history_label);
    
    gtk_box_pack_start(GTK_BOX(mainBox), notebook, 1, 1, 5);
    return mainBox;
}


