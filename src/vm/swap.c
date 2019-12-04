#include "userprog/pagedir.h"
#include "threads/thread.h"
#include "kernel/list.h"
#include "kernel/bitmap.h"
#include "swap.h"
#include "devices/block.h"
#include "page.h"
#include "frame.h"

size_t get_free_slot(size_t size);

void init_swap_block(){
  b = block_get_role(BLOCK_SWAP);
  bmap = bitmap_create(block_size(b));
}

void write_to_swap(struct frame *f){
  struct swap *s;
  s = malloc(sizeof(struct swap));
  if (s == NULL) PANIC("swap fail");
  size_t start = get_free_slot(sizeof(s));
  s->position = start;
  s->frame1 = f;
  list_push_back(&swap_table, &s->s_elem);

  bitmap_set_multiple(bmap, start, sizeof(s), 1);
  block_write(b, (block_sector_t) start, s);
}

void read_from_swap(struct frame* f) {
  struct hash_iterator i;
  hash_first(&i, &thread_current()->spage_table);

  //Remove references to the frame from any page table that refers to it
  while (hash_next(&i)) {
    struct spage *sp = hash_entry (hash_cur(&i), struct spage, pelem);
    if (sp == NULL)
      break;
    if (sp->kpage == f->page) {
      if (!sp->in_swap_table)
        PANIC("Q");
      size_t start = sp->position_in_swap;
      block_read(b, (block_sector_t) start, f);
    }
  }
}



size_t get_free_slot(size_t size){
  return bitmap_scan(bmap, 0, size, 0);
}

