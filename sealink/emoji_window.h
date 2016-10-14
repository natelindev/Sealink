//
//  emoji_window.h
//  sealink
//
//  Created by 林理露 on 16/8/30.
//  Copyright © 2016年 林理露. All rights reserved.
//

#ifndef emoji_window_h
#define emoji_window_h

enum
{
    COL_DISPLAY_NAME,
    COL_NUMBER,
    COL_PIXBUF,
    NUM_COLS
};

typedef struct{
    GtkWidget *window;
    GtkWidget *icon_view;
}emoji_info;

GtkWidget *create_emoji_window(GtkWidget* main_window);
#endif /* emoji_window_h */
