//
// Created by cl10418 on 23/11/19.
//

#ifndef PINTOS_47_FRAME_H
#define PINTOS_47_FRAME_H

#include <stdint.h>

void frame_table_init(void);
void frame_create(uint32_t *page);


#endif //PINTOS_47_FRAME_H
