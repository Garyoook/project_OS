
#include "threads/malloc.h"
#include <debug.h>
#include "threads/thread.h"
#include <string.h>
#include "frame.h"
#include "page.h"
#include "userprog/pagedir.h"
#include "vm/swap.h"
#include "threads/vaddr.h"
#include "lib/random.h"

void frame_delete(struct frame* f);
int ctr = 0;

void frame_table_init() {
  list_init(&frame_table);
  lock_init(&frame_table_lock);
  lock_init(&eviction_lock);
}

void* frame_create(enum palloc_flags flags, struct thread *thread) {

  void * kpage = palloc_get_page(flags);

  if (kpage != NULL) { // palloc success

    // add a new frame table entry
    struct frame *f = malloc(sizeof(struct frame));
    f->kpage = kpage;
    f->t = thread_current();

    if (!lock_held_by_current_thread(&frame_table_lock)) {
      lock_acquire(&frame_table_lock);
    }

    list_push_back(&frame_table, &f->f_elem);
    lock_release(&frame_table_lock);

  } else {
    /* try to evict a page */
    bool evict_success = frame_evict();

    if (evict_success) {
      kpage = palloc_get_page(flags);
      struct frame *new_frame = malloc(sizeof(struct frame));
      new_frame->kpage = kpage;
      new_frame->t = thread_current();
      if (!lock_held_by_current_thread(&frame_table_lock)) {
        lock_acquire(&frame_table_lock);
      }
      list_push_back(&frame_table, &new_frame->f_elem);
      lock_release(&frame_table_lock);
    } else {
      PANIC ("Evict failed");
    }

  }
  return kpage;
}

void frame_delete(struct frame* f) {
  palloc_free_page(f->kpage);
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
    if (f->kpage == frame) {
      return f;
    }
    e = e->next;
  }
  return NULL;
}

struct frame *choose_by_evict_policy();

struct frame *choose_by_evict_policy()
{
  // random number
  int list_length = list_size(&frame_table);
  int r = random_ulong() % list_length;
  if (r < 0) {
    r = -r;
  }

  struct frame * frame_to_evict;

  if (!lock_held_by_current_thread(&frame_table_lock)) {
    lock_acquire(&frame_table_lock);
  }

  struct list_elem * e;
  e = list_begin(&frame_table);
  while (r > 0) {
    e = list_next(e);
    r--;
  }

  frame_to_evict = list_entry(e, struct frame, f_elem);

  lock_release(&frame_table_lock);
  return frame_to_evict;
}


bool frame_evict() {
  bool eviction_success = false;


  struct frame * frame_to_evict;
  frame_to_evict = choose_by_evict_policy();

  // locate the s_page_table_entry
  struct spage * spage = lookup_spage(frame_to_evict->upage);

  // make sure the spage is not evicted already.
  while (spage->evicted == true) {
    frame_to_evict = choose_by_evict_policy();
    spage = lookup_spage(frame_to_evict->upage);
  }

  if (spage == NULL) {
    PANIC("No page is linked with this frame");
  }

  spage->evicted = true;
  struct thread * t = frame_to_evict->t;

  // swap out
  size_t index = swap_index_global;

  read_from_swap (spage->upage, index);

  // remove reference
  pagedir_clear_page(t->pagedir, spage->upage);

  // free the victim
  list_remove(&frame_to_evict->f_elem);
  palloc_free_page(frame_to_evict->kpage);

  spage->is_in_memory = false;
  spage->swapped = true;
  spage->swap_index = swap_index_global;
  spage->evicted = false;

  // success
  return true;

}



