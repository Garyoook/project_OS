//
// Created by wz5918 on 30/11/19.
//

#ifndef PINTOS_47_FRAME_H
#define PINTOS_47_FRAME_H

#include "list.h"
#include "threads/palloc.h"


struct frame{
  void* upage;
  void* kpage;
  struct thread* owner_thread;
  struct list_elem f_elem;
};

struct list frame_table;
void frame_init();
struct frame* frame_create(enum palloc_flags flags, struct thread *t, void* upage);
void frame_evict(void);
struct frame * lookup_frame(void *frame);
void frame_delete(struct frame* frame);
struct list swap_table;

#endif //PINTOS_47_FRAME_H
