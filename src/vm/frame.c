
#include "threads/malloc.h"
#include <debug.h>
#include "threads/thread.h"
#include <string.h>
#include "frame.h"
#include "page.h"
#include "userprog/pagedir.h"
#include "vm/swap.h"
#include "threads/vaddr.h"

void frame_delete(struct frame* f);
int ctr = 0;

void *frame_create(enum palloc_flags flags, void *upage) {
  struct frame *f = malloc(sizeof(struct frame));
  ctr++;
  if (f == NULL) {
    return f;
  }
  f->kpage         = palloc_get_page(flags);
  if (f->kpage == NULL) {
    printf("frame has been used up!!\n");
    frame_evict();
    return NULL;
  }
  f->t            = thread_current();
  f->upage        = upage;
  list_push_back(&frame_table, &f->f_elem);
  return f->kpage;
}

void frame_delete(struct frame* f){
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

void frame_evict() {
  struct list_elem *e = list_head(&frame_table);
  ASSERT(e != NULL);
  struct frame * frame_to_evict = list_entry(e, struct frame, f_elem);
  void *kpage = frame_to_evict->kpage;
  struct spage *sp = lookup_spage(frame_to_evict->upage);
  size_t index = write_to_block(kpage);
//  printf("PPPPPPPPPPPPPPPPPPPPPPPPPPPP   %p\n", frame_to_evict->kpage);
  pagedir_clear_page(thread_current()->pagedir, frame_to_evict->upage);
  list_pop_front(&frame_table);
  sp->kpage = NULL;
  sp->evicted = true;
  sp->reclaim_index = index;
}



