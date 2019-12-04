
#include "threads/malloc.h"
#include <debug.h>
#include "threads/thread.h"
#include <string.h>
#include "userprog/syscall.h"
#include "frame.h"
#include "swap.h"
#include "page.h"
#include "userprog/pagedir.h"

void frame_delete(struct frame* frame1);

void* frame_create(enum palloc_flags flags, struct thread *thread1) {
  struct frame *f = malloc(sizeof(struct frame));
  f->page         = palloc_get_page(flags);
  if (f->page == NULL) {
    eviction();
    f->page = palloc_get_page(flags);printf("Q%d\n", f->page);
  }
  f->t            = thread1;
  list_push_back(&frame_table, &f->f_elem);
  return f->page;
}

void frame_delete(struct frame* frame1){
  palloc_free_page(frame1->page);
  list_remove(&frame1->f_elem);
  free(frame1);
}

void frame_update() {
}

bool eviction() {
  //eviction policy
  struct list_elem *ef = list_pop_front(&frame_table);
  struct frame *evict_frame = list_entry(ef, struct frame, f_elem);
  struct swap *s = write_to_swap(evict_frame);
  palloc_free_page(evict_frame->page);

  struct hash_iterator i;
  hash_first(&i, &thread_current()->spage_table);

  //Remove references to the frame from any page table that refers to it
  while (hash_next(&i)) {
    struct spage *sp = hash_entry (hash_cur(&i), struct spage, pelem);
    if (sp == NULL) {
      break;
    }

    if (sp->kpage == evict_frame->page) {
      sp->in_swap_table = true;
      s->sp = sp;
      return true;
    }
  }
  return false;
}

