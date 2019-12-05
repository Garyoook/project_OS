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

void write_to_swap(void* something){
  size_t start = get_free_slot(sizeof(something));
  bitmap_set_multiple(bmap, start, sizeof(something), 1);
  block_write(b, (block_sector_t) start, something);
}

void read_from_swap(void* something) {
  if (lookup_spage(something)->in_swap_table == false) PANIC("HAHAHAHA");
  size_t  start = lookup_spage(something)->position_in_swap;
  block_read(b, (block_sector_t) start, something);
}

size_t get_free_slot(size_t size){
  return bitmap_scan(bmap, 0, size, 0);
}

