#include "threads/interrupt.h"
#include "kernel/list.h"
#include "kernel/bitmap.h"
#include "swap.h"
#include "devices/block.h"
#include "page.h"

size_t get_free_slot(size_t size);

void init_swap_block(){
  b = block_get_role(BLOCK_SWAP);
  bmap = bitmap_create(block_size(b));
}

void write_to_swap(void* something, struct swap_entry* swapEntry){
  size_t start = get_free_slot(sizeof(something));
  bitmap_set_multiple(bmap, start, sizeof(something), 1);
  swapEntry->blockSector = start;
  block_write(b, (block_sector_t) start, something);
}

void read_from_swap(void* something) {
//  enum intr_level old_level = intr_disable();
  size_t  start = lookup_swap(something)->blockSector;
  block_read(b, (block_sector_t) start, something);
//  intr_set_level(old_level);
}

size_t get_free_slot(size_t size){
  return bitmap_scan(bmap, 0, size, 0);
}

struct swap_entry* lookup_swap(void* upage) {
    struct list_elem *e = list_begin(&swap_table);
    while (e != list_end(&swap_table)){
      struct swap_entry *this_swap = list_entry(e, struct swap_entry, s_elem);
      if (this_swap->uspage == upage) {
        return this_swap;
      }
      e = e->next;
    }
    return NULL;
}

