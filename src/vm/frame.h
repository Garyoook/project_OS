//
// Created by cl10418 on 23/11/19.
//

#ifndef PINTOS_47_FRAME_H
#define PINTOS_47_FRAME_H

#include <stdint.h>
#include "lib/kernel/list.h"

struct frame_entry
{
  uint32_t *page;
  struct thread *td;
  struct list_elem elem;
};

void frame_table_init(void);
struct frame_entry * frame_create(uint32_t *page);


#endif //PINTOS_47_FRAME_H
