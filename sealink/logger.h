//
//  logger.h
//  sealink
//
//  Created by 林理露 on 16/8/28.
//  Copyright © 2016年 林理露. All rights reserved.
//

#ifndef logger_h
#define logger_h

enum{LOG_DEBUG,LOG_INFO,LOG_WARNING,LOG_ERROR};
void logEvent(int level,gchar* log_info,...);

#endif /* logger_h */
