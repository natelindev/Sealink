//
//  capture_image.h
//  sealink
//
//  Created by 林理露 on 16/8/30.
//  Copyright © 2016年 林理露. All rights reserved.
//

#ifndef capture_image_h
#define capture_image_h

void capture_image();

typedef struct
{
    gint x;
    gint y;
    gint width;
    gint height;
    gboolean press;
}DATA;

#endif /* capture_image_h */
