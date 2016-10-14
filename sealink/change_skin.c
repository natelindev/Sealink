//
//  change_skin.c
//  sealink
//
//  Created by 林理露 on 16/9/1.
//  Copyright © 2016年 林理露. All rights reserved.
//

#include <gtk/gtk.h>
#include "change_skin.h"
void change_background(GtkWidget *widget, int w, int h, const gchar *path)
{
    GdkPixbuf *src;
    GdkPixbuf *dst;
    GdkPixmap *pixmap;
    
    gtk_widget_set_app_paintable(widget, TRUE);
    gtk_widget_realize(widget);
    
    gtk_widget_queue_draw(widget);
    src = gdk_pixbuf_new_from_file(path, NULL);
    dst = gdk_pixbuf_scale_simple(src,w,h,GDK_INTERP_BILINEAR);
    
    pixmap = NULL;
    gdk_pixbuf_render_pixmap_and_mask(dst,&pixmap,NULL,5);
    
    gdk_window_set_back_pixmap(widget->window,pixmap,FALSE);

    g_object_unref(src);
    g_object_unref(dst);
    g_object_unref(pixmap);
}