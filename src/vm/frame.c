#include "threads/malloc.h"
#include "threads/thread.h"
#include <string.h>
#include "user/syscall.h"
#include "frame.h"
#include "userprog/pagedir.h"
#include "swap.h"
int k = 0;
struct lock frame_lock;
struct lock eviction_lock;

void frame_delete(struct frame* frame);

struct frame *get_frame_to_evict();

void frame_init() {
  list_init(&frame_table);
  lock_init(&frame_lock);
  lock_init(&eviction_lock);
}

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

  lock_acquire(&frame_lock);
  list_push_back(&frame_table, &f->f_elem);
  lock_release(&frame_lock);

  return f;
}


void frame_delete(struct frame* f){
  lock_acquire(&frame_lock);
  pagedir_clear_page (thread_current()->pagedir, f->upage);

  palloc_free_page(f->kpage);
  list_remove(&f->f_elem);
  free(f);
  lock_release(&frame_lock);
}

struct frame *lookup_frame(void *frame) {
  struct list_elem *e = list_begin(&frame_table);
  while (e != list_end(&frame_table)) {
    struct frame *f = list_entry(e, struct frame, f_elem);
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
  lock_acquire(&eviction_lock);

  struct frame *this_frame = get_frame_to_evict();
  struct swap_entry *swapEntry = malloc(sizeof(struct swap_entry));

  if (swapEntry == NULL) {
    exit(-1);
  }

  swapEntry->uspage = this_frame->upage;
  swapEntry->blockSector =  write_to_swap(this_frame->kpage, swapEntry);
  if (this_frame->upage == 0x8149000) printf("===== evicted here!\n");
  //  printf("Q%x\n", swapEntry->blockSector);
  swapEntry->t_blongs_to = this_frame->owner_thread;
  list_push_back(&swap_table, &swapEntry->s_elem);
  if (this_frame->upage == 0x8149000) swap_debug_dump();
  frame_delete(this_frame);

  lock_release(&eviction_lock);
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
      while (e != list_tail(&frame_table)) {
        f = list_entry(e, struct frame, f_elem);
        t = f->owner_thread;
        bool accessed = pagedir_is_accessed (t->pagedir, f->upage);
        if (!accessed) {
          frame_to_evict = f;
          list_remove (e);
          list_push_back(&frame_table, e);
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

