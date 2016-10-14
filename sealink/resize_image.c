//
//  resize_image.c
//  GTKTest
//
//  Created by 林理露 on 16/8/24.
//  Copyright © 2016年 林理露. All rights reserved.
//
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "logger.h"
#include "resize_image.h"
gboolean resize_image(GtkWidget *image,int width,int height){
    GdkPixbuf *pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(image));
    if (pixbuf == NULL) {
        logEvent(LOG_ERROR, "Failed to resize image.\n");
        return FALSE;
    }
    
    logEvent(LOG_INFO, "Image resized, width: %d,height: %d.\n",width,height);
    pixbuf = gdk_pixbuf_scale_simple(pixbuf, width,height, GDK_INTERP_BILINEAR);
    gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
    return TRUE;
}