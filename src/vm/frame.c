#include "threads/malloc.h"
#include <debug.h>
#include "threads/thread.h"
#include <string.h>
#include "userprog/pagedir.h"
#include "frame.h"
#include "swap.h"
#include "page.h"

void frame_delete(struct frame* frame1);

int block_index = 0;

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

void* frame_evict() {
  struct frame *f = list_entry(list_begin(&frame_table), struct frame, f_elem);
  f->page = NULL;
  pagedir_clear_page(f->t->pagedir, f->page);
  write_to_block(f->frame, block_index);
  struct spage *evicted_page = lookup_spage(f->page);
  evicted_page->block_index = block_index;
  evicted_page->evicted = true;
  block_index++;
  list_remove(&f->f_elem);
  return f;
}

void frame_reclaim(void *frame) {
  struct frame *f = frame;
  f->page = palloc_get_page(PAL_USER);
  f->t = thread_current();
  list_push_back(&frame_table, &f->f_elem);
  struct spage *reclaimed_page = lookup_spage(f->page);
  read_from_block(f, reclaimed_page->block_index);
  reclaimed_page->evicted = false;
}

void frame_update() {
}
