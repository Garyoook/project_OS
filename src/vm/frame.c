
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
    if (eviction()) {
      f->page = palloc_get_page(flags);
    } else {
      exit(-1);
    }
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

  struct hash_iterator i;
  hash_first(&i, &thread_current()->spage_table);

  //Remove references to the frame from any page table that refers to it
  while (hash_next(&i)) {
    struct spage *sp = hash_entry (hash_cur(&i), struct spage, pelem);
    if (sp == NULL)
      break;
    if (sp->kpage == evict_frame->page) {
      pagedir_clear_page(thread_current()->pagedir, sp->upage);
      sp->kpage = NULL;

      if (sp->file1 == NULL) {
        struct swap_entry *swap = swap_create(evict_frame->page, evict_frame->t, evict_frame->frame);
        if (swap == NULL) {
          exit(-1);
        }
      } else {
        munmap(sp->md);
      }
      return true;
    }
  }
  return false;
}

