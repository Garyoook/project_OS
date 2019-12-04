#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"
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

struct swap *write_to_swap(struct frame *f){
  struct swap *s;
  s = malloc(sizeof(struct swap));
  if (s == NULL) PANIC("swap fail");
  size_t start = get_free_slot(sizeof(s));
  s->position = start;
  s->frame1 = f;
  list_push_back(&swap_table, &s->s_elem);

  bitmap_set_multiple(bmap, start, sizeof(s), 1);
  block_write(b, (block_sector_t) start, s);
  return s;
}

void read_from_swap(uint8_t *addr, struct frame *frame) {
  struct swap *temp_s;
  temp_s = malloc(sizeof(struct swap));
  struct list_elem *e = list_begin(&swap_table);

  while (e != list_end(&swap_table)) {
    struct swap *s = list_entry(e, struct swap, s_elem);
    e = e->next;
    if (!s->sp->in_swap_table) {
      exit(-1);
    } else if (s->sp->upage == addr) {
      size_t start = s->position;
      block_read(b, (block_sector_t) start, temp_s);
      install_page(temp_s->sp->upage, frame, temp_s->sp->writable);
    }
  }
}


size_t get_free_slot(size_t size){
  return bitmap_scan(bmap, 0, size, 0);
}

