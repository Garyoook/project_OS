
#include "threads/malloc.h"
#include <debug.h>
#include "threads/thread.h"
#include <string.h>
#include "user/syscall.h"
#include "frame.h"
#include "swap.h"
void frame_delete(struct frame* frame1);

void* frame_create(enum palloc_flags flags, struct thread *thread1) {
  struct frame *f = malloc(sizeof(struct frame));
  f->page         = palloc_get_page(flags);
  f->t            = thread1;
  list_push_back(&frame_table, &f->f_elem);
  if (f->page == NULL) frame_evict(flags, thread1);
//  printf("haha\n");
  return f->page;
}

void frame_delete(struct frame* frame1){
  palloc_free_page(frame1->page);
  list_remove(&frame1->f_elem);
  free(frame1);
}

void frame_evict(enum palloc_flags flags, struct thread *thread1) {
  struct frame *this_frame = list_entry(list_pop_front(&frame_table), struct frame, f_elem);
  write_to_swap(this_frame->frame);
  frame_delete(this_frame);
}


