//
//  settings_window.h
//  SeaLink
//
//  Created by 林理露 on 16/8/28.
//  Copyright © 2016年 林理露. All rights reserved.
//

#ifndef settings_window_h
#define settings_window_h

GtkWidget *create_settings_window();

typedef struct{
    GtkWidget *window;
    GtkWidget *avatar_button;
    GtkWidget *nickname_entry;
    GtkWidget *combo_box;
    GtkWidget *file_save_path_entry;
}set_info_widgets;
#endif /* settings_window_h */
