//
//  capture_image.c
//  sealink
//
//  Created by 林理露 on 16/8/30.
//  Copyright © 2016年 林理露. All rights reserved.
//

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <time.h>
#include "global_settings.h"
#include "capture_image.h"
GdkPixbuf *pixbuf;


//保存鼠标坐标位置的数据结构
void save(GtkWidget *widget,DATA *data)
{
    gchar path[dest_len];
    
    time_t nowtime;
    struct tm *nt;
    time(&nowtime);
    nt = localtime(&nowtime);

    sprintf(path,"%s/screenshots/%d-%02d-%02d %02d:%02d:%02d.jpg",g_s.main_path,nt->tm_year+1900,nt->tm_mon+1,nt->tm_mday,nt->tm_hour,nt->tm_min,nt->tm_sec);
    
    gdk_pixbuf_save (pixbuf,path,"jpeg", NULL, "quality","100", NULL);
}
void show_picture(GdkWindow *window,DATA *data)
//显示截图函数
{
    //显示图片的窗口
    GtkWidget *cap_window;
    GtkWidget *image;
    GtkWidget *save_button,*cancel_button;
    GtkWidget *mainBox;
    GtkWidget *b_box;
    GtkWidget *img_box;
    GtkWidget *frame;
    
    cap_window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(cap_window), GTK_WIN_POS_CENTER);
    gtk_window_set_title(GTK_WINDOW(cap_window),"截取的图片");
    g_signal_connect(G_OBJECT(cap_window),"delete_event", G_CALLBACK(gtk_false),NULL);
    
    gdk_window_set_cursor(gdk_get_default_root_window(), gdk_cursor_new(GDK_LEFT_PTR));
    gdk_flush();
    //恢复鼠标光标图案
    pixbuf=gdk_pixbuf_get_from_drawable(NULL,window,NULL, data->x,data->y,0,0, data->width,data->height);
    //取到矩形区域图片
    image=gtk_image_new_from_pixbuf(pixbuf);
    
    img_box = gtk_hbox_new(FALSE, 0);
    
    frame = gtk_frame_new ("截取的图片");
    b_box = gtk_hbutton_box_new ();
    mainBox=gtk_vbox_new (FALSE,5);
    //gtk_box_pack_start (GTK_BOX (vbox), b_box, FALSE, FALSE, 0);
    gtk_button_box_set_layout (GTK_BUTTON_BOX (b_box), GTK_BUTTONBOX_END);
    save_button=gtk_button_new_with_label ("保存");
    cancel_button=gtk_button_new_with_label ("放弃");
    gtk_signal_connect_object(GTK_OBJECT (save_button),"clicked",G_CALLBACK(save),NULL);
    gtk_signal_connect_object(GTK_OBJECT (save_button),"enter",G_CALLBACK(save),NULL);
    gtk_signal_connect_object(GTK_OBJECT (save_button),"clicked",GTK_SIGNAL_FUNC(gtk_widget_destroy),GTK_OBJECT(cap_window));
    gtk_signal_connect_object(GTK_OBJECT (cancel_button),"clicked",GTK_SIGNAL_FUNC(gtk_widget_destroy),GTK_OBJECT(cap_window));
    
    gtk_box_pack_start(GTK_BOX(b_box), save_button, 0, 0, 5);
    gtk_box_pack_start(GTK_BOX(b_box), cancel_button, 0, 0, 5);
    
    gtk_container_add(GTK_CONTAINER(img_box),image);
    gtk_container_add(GTK_CONTAINER(frame),img_box);
    gtk_box_pack_start (GTK_BOX (mainBox), frame, 1, 1, 10);
    gtk_box_pack_start(GTK_BOX(mainBox), b_box, 0, 0, 5);

    gtk_container_set_border_width(GTK_CONTAINER(mainBox), 20);
    gtk_container_add(GTK_CONTAINER(cap_window),mainBox);
    gtk_widget_show_all(cap_window);
    
}
void select_area_press(GtkWidget *widget,GdkEventButton *event,DATA *data)
//鼠标按下时的操作
{
    if(data->press == TRUE) return;
    //如果当前鼠标已经按下直接返回
    gtk_window_move(GTK_WINDOW(widget),-100,-100);
    //将窗口移出屏幕之外
    gtk_window_resize(GTK_WINDOW(widget),10,10);
    gtk_window_set_opacity(GTK_WINDOW(widget),0.5);
    //设置窗口透明度为80%不透明
    data->press=TRUE;
    data->x=event->x_root; data->y=event->y_root;
    //得到当前鼠标所在坐标
}
void select_area_release(GtkWidget *widget,GdkEventButton *event,DATA *data)
//鼠标释放时操作
{
    if(!data->press) return;
    data->width=ABS(data->x-event->x_root);
    data->height=ABS(data->y-event->y_root);
    //得到当前矩形的宽度和高度
    data->x=MIN(data->x,event->x_root);
    data->y=MIN(data->y,event->y_root);
    //得到当前矩形初始坐标
    data->press=FALSE;
    gtk_widget_destroy(widget);
    gtk_main_quit();
}
void select_area_move(GtkWidget *widget,GdkEventMotion *event,DATA *data)
//鼠标移动时操作
{
    GdkRectangle draw;
    if(!data->press) return;
    draw.width=ABS(data->x-event->x_root);
    draw.height=ABS(data->y-event->y_root);
    draw.x=MIN(data->x,event->x_root);
    draw.y=MIN(data->y,event->y_root);
    //得到当前矩形初始坐标和当前矩形宽度
    if(draw.width <= 0 || draw.height <=0)
    {
        gtk_window_move(GTK_WINDOW(widget),-100,-100);
        gtk_window_resize(GTK_WINDOW(widget),10,10);
        return;
    }
    gtk_window_move(GTK_WINDOW(widget),draw.x,draw.y);
    gtk_window_resize(GTK_WINDOW(widget),draw.width,draw.height);
    //将窗口移动到当前矩形初始坐标处并画出窗口
}
void capture_image()
{
    GtkWidget *win;
    GdkScreen *screen;
//    GdkColor color;
    DATA data;
    screen=gdk_screen_get_default();
    win=gtk_window_new(GTK_WINDOW_POPUP);
    
    gtk_window_set_opacity(GTK_WINDOW(win),0.3);
    //gtk_window_set_transient_for(GTK_WINDOW (win),GTK_WINDOW (main_window));
    gtk_widget_set_app_paintable(win,TRUE);
    data.press=FALSE;
    gtk_widget_add_events(win,GDK_BUTTON_MOTION_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK);
    //添加信号
    g_signal_connect(G_OBJECT(win),"button_press_event", G_CALLBACK(select_area_press),&data);
    g_signal_connect(G_OBJECT(win),"button_release_event", G_CALLBACK(select_area_release),&data);
    g_signal_connect(G_OBJECT(win),"motion_notify_event", G_CALLBACK(select_area_move),&data);
    /*color.blue=0;
     color.green=0;
     color.red=0;*/
    //gtk_widget_modify_bg(win,GTK_STATE_NORMAL,&color);
    //设置背景
    
    //设置窗口全透明
    gtk_window_resize(GTK_WINDOW(win), gdk_screen_get_width(screen), gdk_screen_get_height(screen));
    //设置窗口大小为全屏
    gdk_window_set_cursor(gdk_get_default_root_window(), gdk_cursor_new(GDK_CROSSHAIR));
    gdk_flush();
    //设置并更新鼠标光标图案
    gtk_widget_show_all(win);
    gtk_main();
    g_usleep(10000); //这里要等待一小会，不然截取的图像会有些残影
    show_picture(gdk_get_default_root_window(),&data);
}

