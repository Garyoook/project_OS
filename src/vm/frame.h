//
// Created by wz5918 on 30/11/19.
//

#ifndef PINTOS_47_FRAME_H
#define PINTOS_47_FRAME_H

#include "list.h"
#include "threads/palloc.h"


struct frame{
  void* kpage;
  void* upage;
  struct thread* t;
  struct list_elem f_elem;
};

struct list frame_table;
void* frame_create(void *upage, enum palloc_flags flags, struct thread *thread, bool writable);
void frame_evict(void);
struct frame * lookup_frame(void *frame);

#endif //PINTOS_47_FRAME_H
