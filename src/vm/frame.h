//
// Created by wz5918 on 30/11/19.
//

#ifndef PINTOS_47_FRAME_H
#define PINTOS_47_FRAME_H

#include "list.h"
#include "threads/palloc.h"


struct frame{
  void* frame;
  void* page;
  struct thread* t;
  struct list_elem f_elem;
};

struct list frame_table;
void* frame_create(enum palloc_flags flags, struct thread *thread1);
void frame_delete(struct frame* frame);
void frame_evict();

#endif //PINTOS_47_FRAME_H
