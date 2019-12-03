#include "swap.h"
#include "devices/block.h"
#include "kernel/bitmap.h"
#include "threads/vaddr.h"

static struct block *swap_block;

void swap_init() {
  swap_index = 0;
  swap_block = block_get_role(BLOCK_SWAP);
  swap_bmap = bitmap_create(block_size(swap_block));
}

void read_from_block(void* frame, int index) {
  for(int i = 0; i < PGSIZE/BLOCK_SECTOR_SIZE; i++) {
    block_read(swap_block, (block_sector_t) (index + i), frame + (i * BLOCK_SECTOR_SIZE));
  }
}

void write_to_block(void* frame, int index) {
  for(int i = 0; i < PGSIZE/BLOCK_SECTOR_SIZE; i++) {
    block_write(swap_block, (block_sector_t) (index + i), frame + (i * BLOCK_SECTOR_SIZE));
  }
}

void to_swap(void *kpage) {
  size_t index = bitmap_scan_and_flip(swap_bmap, 0, 1, true);
  if (index == BITMAP_ERROR) {
    return;
  }
  block_sector_t ofs = index * (PGSIZE/BLOCK_SECTOR_SIZE);
  write_to_block(kpage, ofs);
}

void from_swap(void *kpage) {
  size_t index = bitmap_scan_and_flip(swap_bmap, (tbd), (tbd), true);
  if (index == BITMAP_ERROR) {
    return;
  }
  block_sector_t ofs = index * (PGSIZE/BLOCK_SECTOR_SIZE);
  read_from_block(kpage, ofs);
}