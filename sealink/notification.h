//
//  notification.h
//  sealink
//
//  Created by 林理露 on 16/8/31.
//  Copyright © 2016年 林理露. All rights reserved.
//

#ifndef notification_h
#define notification_h

enum{ON,OFF,F_REC,MSG,GMSG,FRIEND};

void addnotify(int Mode, simple_user_info *u_info);
void notify(int Mode,simple_user_info *u_info);

#endif /* notification_h */
