
#include "threads/malloc.h"
#include <debug.h>
#include "threads/thread.h"
#include <string.h>
#include "frame.h"
#include "page.h"
#include "userprog/pagedir.h"
#include "vm/swap.h"

void frame_delete(struct frame* f);
int ctr = 0;

void* frame_create(enum palloc_flags flags, struct thread *thread) {
  struct frame *f = malloc(sizeof(struct frame));
//  printf("creating frame...%d\n", ctr);
  ctr++;
  if (f == NULL) {
    return f;
  }
  f->page         = palloc_get_page(flags);
  if (f->page == NULL) {
//    printf("frame has been used up!!\n");
    return NULL;
  }
  f->t            = thread;
  list_push_back(&frame_table, &f->f_elem);
  return f->page;
}

void frame_delete(struct frame* f){
  palloc_free_page(f->page);
  list_remove(&f->f_elem);
  free(f);
}

void frame_update() {
}

struct frame * lookup_frame(void *frame) {
  struct list_elem *e = list_begin(&frame_table);
  while (e != list_end(&frame_table)) {
    struct frame * f = list_entry(e, struct frame, f_elem);
    if (f == NULL) {
      return NULL;
    }
    if (f->page == frame) {
      return f;
    }
    e = e->next;
  }
  return NULL;
}

void frame_evict(void *kpage) {
  struct list_elem *e = list_pop_front(&frame_table);
  struct frame * frame_to_evict = list_entry(e, struct frame, f_elem);

//  struct frame *frame_to_evict = lookup_frame(kpage);
  struct spage *sp = lookup_spage(frame_to_evict->page);
  pagedir_clear_page(thread_current()->pagedir, frame_to_evict->page);
  list_remove(&frame_to_evict->f_elem);
  sp->kpage = NULL;
  sp->evicted = true;
  swap_index++;
  sp->reclaim_index = 0;
}



