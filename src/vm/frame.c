
#include "threads/malloc.h"
#include <debug.h>
#include "threads/thread.h"
#include <string.h>
#include "user/syscall.h"
#include "frame.h"
#include "swap.h"
void frame_delete(struct frame* frame1);

void* frame_create(enum palloc_flags flags, struct thread *thread1, void* upage) {
    struct frame *f = malloc(sizeof(struct frame));
    f->page         = palloc_get_page(flags);
    if (f->page == NULL) {
        frame_evict();
        f->page = palloc_get_page(flags);
//        printf("Q%d\n", f->page);
    }
    f->t            = thread1;
    f->upage        = upage;
    list_push_back(&frame_table, &f->f_elem);
    return f->page;
}


void frame_delete(struct frame* frame1){
  palloc_free_page(frame1->page);
  list_remove(&frame1->f_elem);
  free(frame1);
}

void frame_evict() {
  struct frame *this_frame = list_entry(list_pop_front(&frame_table), struct frame, f_elem);
  write_to_swap(this_frame->page);
  frame_delete(this_frame);
}


