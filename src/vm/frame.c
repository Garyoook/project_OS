
#include "threads/malloc.h"
#include <debug.h>
#include "threads/thread.h"
#include <string.h>
#include "user/syscall.h"
#include "frame.h"
#include "userprog/pagedir.h"
#include "swap.h"
int k = 0;
void frame_delete(struct frame* f);
struct frame *get_frame_to_evict();

struct frame* frame_create(enum palloc_flags flags, struct thread *thread1, void* upage) {
    struct frame *f = malloc(sizeof(struct frame));
    f->kpage         = palloc_get_page(flags);

    f->owner_thread            = thread1;
    f->upage        = upage;
  if (f->kpage == NULL) {
    frame_evict();
    f->kpage = palloc_get_page(flags);
//    printf("Q%x\n", f->page);
  }
    list_push_back(&frame_table, &f->f_elem);
    return f;
}


void frame_delete(struct frame* f){
  pagedir_clear_page (thread_current()->pagedir, f->upage);

  palloc_free_page(f->kpage);
  list_remove(&f->f_elem);
  free(f);
}

struct frame * lookup_frame(void *frame) {
  struct list_elem *e = list_begin(&frame_table);
  while (e != list_end(&frame_table)) {
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
  struct frame *this_frame = get_frame_to_evict();
  struct swap_entry *swapEntry = malloc(sizeof(struct swap_entry));
  swapEntry->uspage = this_frame->upage;
  swapEntry->blockSector =  write_to_swap(this_frame->kpage, swapEntry);
//  printf("Q%x\n", swapEntry->blockSector);
  swapEntry->t_blongs_to = this_frame->owner_thread;
  list_push_back(&swap_table, &swapEntry->s_elem);
  frame_delete(this_frame);
}

struct frame *get_frame_to_evict() {
    struct frame *f;
    struct thread *t;
    struct list_elem *e;

    struct frame *frame_to_evict = NULL;

    int round_count = 1;
    bool found = false;
    /* iterate each entry in frame table */
    while (!found) {
      e = list_begin(&frame_table);
      while (e != list_tail(&frame_table))
      {
        f = list_entry (e, struct frame, f_elem);
        t = f->owner_thread;
        bool accessed  = pagedir_is_accessed (t->pagedir, f->upage);
//        printf("[ Debugging info ]recefenced : %d\n", accessed);
        if (!accessed)
        {
          frame_to_evict = f;
          list_remove (e);
          list_push_back (&frame_table, e);
          break;
        } else {
          pagedir_set_accessed (t->pagedir, f->upage, false);
        }
      }

      if (frame_to_evict != NULL)
        found = true;
      round_count++;
      if (round_count == 2)
        found = true;
    }

    return frame_to_evict;
}

