
#include <gtk/gtk.h>
#include <time.h>
#include <gdk/gdkkeysyms.h>
/*定义日历结构体*/
#define DEF_PAD 10
#define DEF_PAD_SMALL 5
#define TM_YEAR_BASE 1900
gint calendar_flag=0;
typedef struct _CalendarData {
    GtkWidget *flag_checkboxes[5];
    gboolean  settings[5];
    gchar     *font;
    GtkWidget *font_dialog;
    GtkWidget *window;
    GtkWidget *prev2_sig;
    GtkWidget *prev_sig;
    GtkWidget *last_sig;
    GtkWidget *month;
} CalendarData;

/*GtkCalendar 日历构件
 */

void calendar_date_to_string( CalendarData *data,
                             char         *buffer,
                             gint          buff_len )
{
    
    struct tm *nt;
    time_t nowtime;
    time(&nowtime);
    nt = localtime(&nowtime);
    
    
    gtk_calendar_get_date (GTK_CALENDAR (data->window),
                           (guint*)&nt->tm_year, (guint*)&nt->tm_mon, (guint*)&nt->tm_mday);
    nt->tm_year -= TM_YEAR_BASE;
    nowtime = mktime (nt);
    strftime (buffer, buff_len-1, "%x", gmtime (&nowtime));
}

void calendar_set_signal_strings( char         *sig_str,
                                 CalendarData *data)
{
    const gchar *prev_sig;
    
    prev_sig = gtk_label_get_text (GTK_LABEL (data->prev_sig));
    gtk_label_set_text (GTK_LABEL (data->prev2_sig), prev_sig);
    
    prev_sig = gtk_label_get_text (GTK_LABEL (data->last_sig));
    gtk_label_set_text (GTK_LABEL (data->prev_sig), prev_sig);
    gtk_label_set_text (GTK_LABEL (data->last_sig), sig_str);
}

void calendar_month_changed( GtkWidget    *widget,
                            CalendarData *data )
{
    char buffer[256] = "month_changed: ";
    
    calendar_date_to_string (data, buffer+15, 256-15);
    calendar_set_signal_strings (buffer, data);
}

void calendar_day_selected( GtkWidget    *widget,
                           CalendarData *data )
{
    char buffer[256] = "day_selected: ";
    
    calendar_date_to_string (data, buffer+14, 256-14);
    calendar_set_signal_strings (buffer, data);
}

void calendar_prev_month( GtkWidget    *widget,
                         CalendarData *data )
{
    char buffer[256] = "prev_month: ";
    
    calendar_date_to_string (data, buffer+12, 256-12);
    calendar_set_signal_strings (buffer, data);
}

void calendar_next_month( GtkWidget    *widget,
                         CalendarData *data )
{
    char buffer[256] = "next_month: ";
    
    calendar_date_to_string (data, buffer+12, 256-12);
    calendar_set_signal_strings (buffer, data);
}

void calendar_prev_year( GtkWidget    *widget,
                        CalendarData *data )
{
    char buffer[256] = "prev_year: ";
    
    calendar_date_to_string (data, buffer+11, 256-11);
    calendar_set_signal_strings (buffer, data);
}

void calendar_next_year( GtkWidget    *widget,
                        CalendarData *data )
{
    char buffer[256] = "next_year: ";
    
    calendar_date_to_string (data, buffer+11, 256-11);
    calendar_set_signal_strings (buffer, data);
}


void calendar_set_flags( CalendarData *calendar )
{
    gint i;
    gint options = 0;
    for (i = 0; i < 5; i++)
        if (calendar->settings[i])
        {
            options=options + (1<<i);
        }
    if (calendar->window)
        gtk_calendar_display_options (GTK_CALENDAR (calendar->window), options);
}

//绘制日历构件，并且添加回调函数

GtkWidget* create_calendar_window(GtkWidget* main_window)
{
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *hbbox;
    GtkWidget *calendar;
    GtkWidget *button;
    GtkWidget *frame;
    GtkWidget *bbox;
    static CalendarData calendar_data;
    gint i;
    
    calendar_data.window = NULL;
    calendar_data.font = NULL;
    calendar_data.font_dialog = NULL;
    
    for (i = 0; i < 5; i++) {
        calendar_data.settings[i] = 1;
    }
    calendar_data.settings[2] = 0;
    calendar_data.settings[3] = 0;
    
    window = gtk_window_new (GTK_WINDOW_POPUP);
    gtk_window_set_transient_for(GTK_WINDOW (window),GTK_WINDOW (main_window));
    GTK_WINDOW(window)->destroy_with_parent = TRUE;
    GTK_WINDOW(window)->position = GTK_WIN_POS_CENTER_ON_PARENT;
    
    g_signal_connect (G_OBJECT (window), "destroy",
                      G_CALLBACK (gtk_widget_destroyed),
                      NULL);
    
    
    gtk_window_set_title (GTK_WINDOW (window), "日历");
    gtk_container_set_border_width (GTK_CONTAINER (window), 5);
    
    g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (gtk_false),NULL);
    
    gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
    
    vbox = gtk_vbox_new (FALSE, DEF_PAD);
    gtk_container_add (GTK_CONTAINER (window), vbox);
    
    /*
     * 顶级窗口，其中包含日历构件，设置日历各参数的复选按钮和设置字体的按钮
     */
    
    hbox = gtk_hbox_new (FALSE, DEF_PAD);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, DEF_PAD);
    hbbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (hbox), hbbox, FALSE, FALSE, DEF_PAD);
    gtk_button_box_set_layout (GTK_BUTTON_BOX(hbbox), GTK_BUTTONBOX_SPREAD);
    gtk_box_set_spacing (GTK_BOX (hbbox), 5);
    
    /* 日历构件 */
    frame = gtk_frame_new ("日历");
    gtk_box_pack_start (GTK_BOX (hbbox), frame, FALSE, TRUE, DEF_PAD);
    calendar=gtk_calendar_new ();
    calendar_data.window = calendar;
    calendar_set_flags (&calendar_data);
    gtk_container_add( GTK_CONTAINER (frame), calendar);
    g_signal_connect (G_OBJECT (calendar), "month_changed",
                      G_CALLBACK (calendar_month_changed),
                      &calendar_data);
    g_signal_connect (G_OBJECT (calendar), "day_selected",
                      G_CALLBACK (calendar_day_selected),
                      &calendar_data);
    g_signal_connect (G_OBJECT (calendar), "prev_month",
                      G_CALLBACK (calendar_prev_month),
                      &calendar_data);
    g_signal_connect (G_OBJECT (calendar), "next_month",
                      G_CALLBACK (calendar_next_month),
                      &calendar_data);
    g_signal_connect (G_OBJECT (calendar), "prev_year",
                      G_CALLBACK (calendar_prev_year),
                      &calendar_data);
    g_signal_connect (G_OBJECT (calendar), "next_year",
                      G_CALLBACK (calendar_next_year),
                      &calendar_data);
    
    /*
     *  创建关闭按钮
     */
    
    bbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (vbox), bbox, 0, 0, 0);
    gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END);
    
    button = gtk_button_new_with_label ("关闭");
    
    gtk_signal_connect_object(GTK_OBJECT (button),
                              "clicked",
                              GTK_SIGNAL_FUNC(gtk_widget_destroy), 
                              GTK_OBJECT(window));
    
    gtk_container_add (GTK_CONTAINER (bbox), button);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);
    
    gtk_widget_show_all (window);
    return window;
}
