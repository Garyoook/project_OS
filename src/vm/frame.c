
#include "threads/malloc.h"
#include <debug.h>
#include "threads/thread.h"
#include <string.h>
#include "frame.h"
#include "swap.h"
void frame_delete(struct frame* frame1);

void* frame_create(enum palloc_flags flags, struct thread *thread1) {
  struct frame *f = malloc(sizeof(struct frame));
  f->page         = palloc_get_page(flags);
  f->t            = thread1;
  list_push_back(&frame_table, &f->f_elem);
  return f->page;
}

void frame_delete(struct frame* frame1){
  palloc_free_page(frame1->page);
  list_remove(&frame1->f_elem);
  free(frame1);
}

void frame_evict() {
  struct frame *f = list_entry(list_begin(&frame_table), struct frame, f_elem);
  write_to_block(f->frame, 0);
  palloc_free_page(f->page);

}

void frame_update() {
}
