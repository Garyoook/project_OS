//
// Created by cl10418 on 23/11/19.
//

#ifndef PINTOS_47_FRAME_H
#define PINTOS_47_FRAME_H

#include <stdint.h>
#include "lib/kernel/list.h"
#include "filesys/off_t.h"

struct frame_entry
{
  uint32_t *page;
  off_t offset;
  struct file *file;
  struct thread *td;
  struct list_elem elem;
};

void frame_table_init(void);
struct frame_entry * frame_create(uint32_t *page);


#endif //PINTOS_47_FRAME_H
