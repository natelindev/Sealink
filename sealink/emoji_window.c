//
//  emoji_window.c
//  sealink
//
//  Created by 林理露 on 16/8/30.
//  Copyright © 2016年 林理露. All rights reserved.
//
#include <gtk/gtk.h>
#include "global_settings.h"
#include "emoji_window.h"
#include "callbacks.h"
#include "logger.h"

GtkTreeModel * init_model(void)
{
    GtkListStore *list_store;
    GdkPixbuf *p_em;
    GtkTreeIter iter;
    GError *err = NULL;
    
    gint i = 0;
    gchar path[dest_len];
    list_store = gtk_list_store_new(NUM_COLS,G_TYPE_STRING,G_TYPE_UINT, GDK_TYPE_PIXBUF);
    
    for (i = 1; i <= 42; ++i) {
        sprintf(path, "%s/emoji/%d.jpg",g_s.main_path,i);
        p_em = gdk_pixbuf_new_from_file(path, &err);
        p_em = gdk_pixbuf_scale_simple(p_em, 36,36, GDK_INTERP_BILINEAR);
        if (err == NULL) {
            gtk_list_store_append(list_store, &iter);
            gtk_list_store_set(list_store, &iter,COL_DISPLAY_NAME,NULL,COL_NUMBER,i, COL_PIXBUF, p_em, -1);
        }else{
            logEvent(LOG_ERROR, "FAILED to load emoji %d.\n",i);
        }
    }
    return GTK_TREE_MODEL(list_store);
}

GtkWidget *create_emoji_window(GtkWidget* main_window)
{
    
    GtkWidget *mainBox;
    GtkWidget *sw;
    GtkWidget *bbox;
    GtkWidget *emoji_confirm_button;
    GtkWidget *emoji_cancenl_button;
    emoji_info *my_emoji_info;
    my_emoji_info = g_malloc(sizeof(emoji_info));
    
    if (my_emoji_info == NULL) {
        logEvent(LOG_ERROR, "pointer [my_emoji_info] malloc FAILED.\n");
    }else{
        logEvent(LOG_DEBUG, "pointer [my_emoji_info] MALLOCED, size: %d.\n",sizeof(emoji_info));
    }
    
    my_emoji_info->window = gtk_window_new(GTK_WINDOW_POPUP);
    bbox = gtk_hbutton_box_new();
    mainBox = gtk_vbox_new(FALSE, 0);
    emoji_confirm_button = gtk_button_new_with_label("确定");
    emoji_cancenl_button = gtk_button_new_with_label("取消");

    g_signal_connect(G_OBJECT(emoji_confirm_button),"clicked",G_CALLBACK(on_emoji_confrim_btn_clicked),my_emoji_info);
    
    gtk_signal_connect_object(GTK_OBJECT (emoji_cancenl_button),
                              "clicked",
                              GTK_SIGNAL_FUNC(gtk_widget_destroy),
                              GTK_OBJECT(my_emoji_info->window));
    
    gtk_box_pack_start(GTK_BOX(bbox), emoji_confirm_button, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(bbox), emoji_cancenl_button, 0, 0, 5);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_SPREAD);
    
    gtk_window_set_title(GTK_WINDOW (my_emoji_info->window), "选择表情");
    gtk_window_set_transient_for(GTK_WINDOW(my_emoji_info->window),GTK_WINDOW(main_window));
    gtk_window_set_position(GTK_WINDOW(my_emoji_info->window), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_container_set_border_width(GTK_CONTAINER(my_emoji_info->window), 10);
    gtk_widget_set_size_request(my_emoji_info->window, 300, 300);
    
    sw = gtk_scrolled_window_new(NULL, NULL);
    
    gtk_box_pack_end(GTK_BOX(mainBox), bbox, 0, 0, 5);
    gtk_container_add(GTK_CONTAINER (my_emoji_info->window), mainBox);
    gtk_container_add(GTK_CONTAINER(mainBox), sw);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                        GTK_SHADOW_IN);
    
    my_emoji_info->icon_view = gtk_icon_view_new_with_model(init_model());
    gtk_container_add(GTK_CONTAINER(sw), my_emoji_info->icon_view);
    
    gtk_icon_view_set_text_column(GTK_ICON_VIEW(my_emoji_info->icon_view),
                                  COL_DISPLAY_NAME);
    gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(my_emoji_info->icon_view),
                                    COL_PIXBUF);
    gtk_icon_view_set_selection_mode(GTK_ICON_VIEW(my_emoji_info->icon_view),
                                     GTK_SELECTION_MULTIPLE);
    
    gtk_widget_show_all(my_emoji_info->window);
    return my_emoji_info->window;
}
