//
//  logger.c
//  sealink
//
//  Created by 林理露 on 16/8/28.
//  Copyright © 2016年 林理露. All rights reserved.
//
#include <gtk/gtk.h>
#include <time.h>
#include "logger.h"

gchar *levels[4] = {"DBUG","INFO","WARN","ERRO"};

void logEvent(int level,gchar* log_info,...){
    time_t nowtime;
    struct tm *nt;
    time(&nowtime);
    nt = localtime(&nowtime);
    va_list args;
    g_print("[%s] %d-%02d-%02d %02d:%02d:%02d ",levels[level],nt->tm_year+1900,nt->tm_mon+1,nt->tm_mday,nt->tm_hour,nt->tm_min,nt->tm_sec);
    va_start(args, log_info);
    vprintf(log_info,args);
    va_end(args);
}