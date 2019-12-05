
#include "threads/malloc.h"
#include <debug.h>
#include "threads/thread.h"
#include <string.h>
#include "user/syscall.h"
#include "frame.h"
#include "userprog/pagedir.h"
#include "swap.h"
int k = 0;
void frame_delete(struct frame* frame1);

void* frame_create(enum palloc_flags flags, struct thread *thread1, void* upage) {
    struct frame *f = malloc(sizeof(struct frame));
    f->page         = palloc_get_page(flags);

    f->t            = thread1;
    f->upage        = upage;
  if (f->page == NULL) {
    frame_evict();
    f->page = palloc_get_page(flags);
    printf("Q%x\n", f->page);
  }
    list_push_back(&frame_table, &f->f_elem);
    return f->page;
}


void frame_delete(struct frame* frame1){
  palloc_free_page(frame1->page);
  pagedir_clear_page (thread_current()->pagedir, frame1->upage);
  list_remove(&frame1->f_elem);
  free(frame1);
}

void frame_evict() {
  struct frame *this_frame = list_entry(list_pop_back(&frame_table), struct frame, f_elem);
  struct swap_entry *swapEntry = malloc(sizeof(struct swap_entry));
  write_to_swap(this_frame->page, swapEntry);
  swapEntry->uspage = this_frame->upage;
//  printf("Q%x\n", swapEntry->uspage);
  swapEntry->t_blongs_to = this_frame->t;
  list_push_back(&swap_table, &swapEntry->s_elem);
  frame_delete(this_frame);
}


