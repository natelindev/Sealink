//
//  group_dao.h
//  sealink
//
//  Created by 林理露 on 16/8/31.
//  Copyright © 2016年 林理露. All rights reserved.
//

#ifndef group_dao_h
#define group_dao_h

typedef struct{
    int GID;
}group_info;

GList *get_group_list();
#endif /* group_dao_h */
