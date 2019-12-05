#include "swap.h"
#include "devices/block.h"
#include "kernel/bitmap.h"
#include "threads/vaddr.h"
#include "threads/synch.h"

#define SECTORS_PER_PAGE 8

void swap_init() {
  swap_index_global = 0;
  swap_block = block_get_role(BLOCK_SWAP);
  if (swap_block == NULL) {
    return;
  }
  swap_bmap = bitmap_create(block_size(swap_block)/SECTORS_PER_PAGE);
  lock_init(&swap_lock);
}

size_t write_to_swap(void *kpage) {
  size_t start = bitmap_scan_and_flip(swap_bmap, 0, 1, true);
  if (start == BITMAP_ERROR) {
    return false;
  }
  block_sector_t sector_no = (block_sector_t) (start * SECTORS_PER_PAGE);

  for(int i = 0; i < SECTORS_PER_PAGE; i++) {
    block_write(swap_block, (block_sector_t) sector_no + i, kpage + (i * BLOCK_SECTOR_SIZE));
  }
  swap_index_global = sector_no;              // probably wrong.
  return start;
}

size_t read_from_swap(void *kpage, size_t start) {
  block_sector_t sector_no = (block_sector_t) (start * SECTORS_PER_PAGE);
  for(int i = 0; i < SECTORS_PER_PAGE; i++) {
    block_read(swap_block, (block_sector_t) (sector_no + i), kpage + (i * BLOCK_SECTOR_SIZE));
  }
  bitmap_flip(swap_bmap, start);

  return true;
}
