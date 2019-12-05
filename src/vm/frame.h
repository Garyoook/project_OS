//
// Created by wz5918 on 30/11/19.
//

#ifndef PINTOS_47_FRAME_H
#define PINTOS_47_FRAME_H

#include "list.h"
#include "threads/palloc.h"


struct frame{
  void* upage;
  void* page;
  struct thread* t;
  struct list_elem f_elem;
};

struct list frame_table;
void* frame_create(enum palloc_flags flags, struct thread *thread1, void* upage);
void frame_evict(void);
void frame_delete(struct frame* frame1);
struct list swap_table;

#endif //PINTOS_47_FRAME_H
