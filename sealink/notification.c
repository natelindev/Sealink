//
//  notification.c
//  sealink
//
//  Created by 林理露 on 16/8/31.
//  Copyright © 2016年 林理露. All rights reserved.
//

#include <gtk/gtk.h>
#include <unistd.h>
#include "user_dao.h"
#include "global_settings.h"
#include "notification.h"

struct notify_t {
	int Mode;
	simple_user_info* uinfo;
} ;

static int watcherStart = 0;

struct notify_t g_notify = { -1, NULL };

gboolean notifyWatcher(void* p);

void addnotify(int Mode, simple_user_info *u_info) {
	// I have no time caring about race conditions, fuck this.
	if( watcherStart == 0 ) {
		g_timeout_add(100, notifyWatcher, NULL);
		watcherStart = 1;
	}
	if(g_notify.Mode != -1) return;
	g_notify.Mode = Mode;
	g_notify.uinfo = u_info;
	return;
}

gboolean notifyWatcher(void* p) {

	if( g_notify.Mode != -1 ) {
		notify(g_notify.Mode, g_notify.uinfo);
		g_notify.Mode = -1;
		g_notify.uinfo = NULL;
	}
	return TRUE;
}

//定时器回调函数，返回FALSE只执行一次
gboolean destroyWindow(GtkWidget *window){
    
    gtk_widget_destroy(window);
    return FALSE;
}

//生成通知窗口
void notify(int Mode,simple_user_info *u_info)
{
    
    //定义变量
    GtkWidget * win;  //窗口
    GtkWidget *label;  //显示的文字标签
    char text[dest_len];  //存储文字的指针
    
    //创建一个POPUP式的窗口
    win= gtk_window_new(GTK_WINDOW_POPUP);
    
    //连接信号"delete_event",使得窗口关闭时发生
    g_signal_connect_swapped (G_OBJECT (win), "delete_event",
                              G_CALLBACK (gtk_widget_destroy),win);
    //设定窗口的默认宽高
    gtk_window_set_default_size(GTK_WINDOW(win),200,100);
    //设定窗口显示的位置
    gtk_window_set_position(GTK_WINDOW(win),GTK_WIN_POS_CENTER);
    //得到screen的尺寸
    GdkScreen* cur_screen = NULL;
    cur_screen = gdk_screen_get_default (); //get screen
    int width = gdk_screen_get_width (cur_screen); //get screen width
    int height = gdk_screen_get_height (cur_screen); //get screen height
    //调整窗口的位置
    gtk_window_move((GtkWindow*)win,width-200,height-100);
    //通过Mode来确定是哪种通知
    switch(Mode){
            
        case ON:
            sprintf(text,"%s(%u) is online", u_info->uname, u_info->UID);
            break;
        case OFF:
            sprintf(text , "%s(%u) is offline",u_info->uname, u_info->UID);
            break;
        case F_REC:
            sprintf(text, "%s(%u) sent you a file",u_info->uname, u_info->UID);
            break;
        case MSG:
            sprintf(text, "%s(%u) sent you a message",u_info->uname, u_info->UID);
            break;
	case GMSG:
	    sprintf(text, "New message in group(%u)", u_info->UID);
	    break;
	case FRIEND:
	    sprintf(text, "%s(%d) added you to his friend list", u_info->uname, u_info->UID);
	    break;
    }
    free(u_info);  	// mem leak prevention ?
    //将消息放入标签
    label=gtk_label_new(text);
    //放入窗口
    gtk_container_add (GTK_CONTAINER (win), label);
    //显示
    gtk_widget_show_all(win);
    //定时自动消息
    g_timeout_add(2000, (GSourceFunc)destroyWindow, win);
}
