#ifndef __CALLBACKS_H__
#define __CALLBACKS_H__

void appendText(GtkWidget* text_view, gchar* text);
/****   登录窗口   ****/
void on_login_btn_clicked(GtkWidget *button,gpointer *my_login_info);
void on_register_btn_clicked();
gboolean on_login_window_delete(GtkWidget* widget);

/****   注册界面   ****/
//确认注册按钮
void on_reg_confirm_btn_clicked(GtkWidget*button,gpointer *my_reg_info);
//取消注册按钮
void on_reg_cancel_btn_clicked(GtkWidget*button,gpointer *my_reg_info);
gboolean on_reg_window_delete(GtkWidget* widget);

/****   主界面   ****/
gboolean on_main_window_delete(GtkWidget *widget,GdkEvent *event,gpointer data);
//主界面 HeadBar
void on_avatar_clicked();
void on_status_changed(gchar* status);
//主界面 NavBar
void on_dialog_list_btn_clicked();
void on_friend_list_btn_clicked();
void on_group_list_btn_clicked();
//主界面 List
void on_dialog_clicked(gchar* target_name);
void on_friend_clicked(gchar* friend_name);
void on_group_clicked(gchar* group_name);
//主界面 BottomBar
void on_settings_btn_clicked();
void on_calendar_btn_clicked(GtkWidget* button,GtkWidget* window);


/****   聊天窗口   ****/
//聊天窗口 HeadBar
void on_file_tranfer_btn_clicked(GtkWidget* button,gchar* target_IP);
void on_chat_history_btn_clicked(GtkWidget* button,gchar* target_name,GtkWidget *sidebar);
void on_detail_info_btn_clicked(gchar* target_name,GtkWidget *sidebar);

//聊天窗口 ToolBar
void on_text_style_btn_clicked (GtkWidget* button,gpointer* style);
void on_text_color_btn_clicked (GtkWidget* button,gpointer* color);
void on_face_btn_clicked(GtkWidget* button,GtkWidget* main_window);
void on_screen_shot_btn_clicked();
void on_send_image_btn_clicked(GtkWidget* button,gchar* target_name);

//表情选择窗口
void on_emoji_confrim_btn_clicked(GtkWidget *button,gpointer *my_emoji_info);
//聊天窗口 BottomBar
void on_chat_send_btn_clicked(GtkWidget* button, gpointer* m_info);
void on_chat_close_btn_clicked(GtkWidget* button,GtkWidget* window);
void on_group_chat_send_btn_clicked(GtkWidget* button,gpointer* g_info);

void on_file_rec_btn_clicked(GtkWidget* button,GtkWidget *window);
/****   设置窗口   ****/
void on_set_save_btn_clicked(GtkWidget* button,gpointer* n_s);
void on_set_cancel_btn_clicked(GtkWidget* button,GtkWidget* window);

void on_set_avatar_btn_clicked(GtkWidget* button,GtkWidget* set_window);
void on_set_avatar_confrim_btn_clicked(GtkWidget *button,gpointer* avatar_info);

void on_add_friend_btn_clicked(GtkWidget* button,GtkWidget* entry);
void on_del_friend_btn_clicked(GtkWidget* button,GtkWidget* entry);
void on_add_group_btn_clicked(GtkWidget *button,GtkWidget* entry);

void on_skin_change_btn_clicked(GtkWidget *button, GtkWidget* window);
#endif

