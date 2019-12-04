
#include "threads/malloc.h"
#include <debug.h>
#include "threads/thread.h"
#include <string.h>
#include "frame.h"
#include "page.h"
#include "userprog/pagedir.h"
#include "vm/swap.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "userprog/syscall.h"

void frame_delete(struct frame* f);
int ctr = 0;
int ectr = 0;

void* frame_create(void *upage, enum palloc_flags flags, struct thread *thread, bool writable) {
  struct frame *f = malloc(sizeof(struct frame));
  ctr++;
  if (f == NULL) {
    return f;
  }
  f->kpage = palloc_get_page(flags);
  if (f->kpage == NULL) {
    printf("frame has been used up!!\n");
    frame_evict();
  }
  f->t            = thread;

  if (!install_page(f->upage, f->kpage, writable)) {
    palloc_free_page(f->kpage);
    exit(EXIT_FAIL);
  }
  f->upage = upage;
  list_push_back(&frame_table, &f->f_elem);
  return f->kpage;
}

void frame_delete(struct frame* f){
  palloc_free_page(f->kpage);
  pagedir_clear_page(thread_current()->pagedir, f->kpage);
  list_remove(&f->f_elem);
  free(f);
}

void frame_update() {
}

struct frame * lookup_frame(void *frame) {
  struct list_elem *e = list_begin(&frame_table);
  while (e != list_tail(&frame_table)) {
    struct frame * f = list_entry(e, struct frame, f_elem);
    if (f == NULL) {
      return NULL;
    }
    if (f->kpage == frame) {
      return f;
    }
    e = e->next;
  }
  return NULL;
}

void frame_evict() {
  printf("evict: %d\n", ectr);
  ectr++;
  struct list_elem *e = list_begin(&frame_table);
  struct frame * frame_to_evict = list_entry(e, struct frame, f_elem);
  void *kpage = frame_to_evict->kpage;
  struct spage *sp = lookup_spage(frame_to_evict->upage);
  write_to_swap(kpage);
  frame_delete(frame_to_evict);
  list_remove(e);
  sp->kpage = NULL;
  sp->evicted = true;
  swap_index++;
  sp->reclaim_index = swap_index;
}



