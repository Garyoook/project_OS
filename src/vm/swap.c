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

bool write_to_swap(void *kpage) {
  size_t start = bitmap_scan_and_flip(swap_bmap, 0, 1, true);
  if (start == BITMAP_ERROR) {
    return false;
  }
  block_sector_t ofs = (block_sector_t) (start * (PGSIZE / BLOCK_SECTOR_SIZE));
  write_to_block(kpage, ofs);
  swap_index = ofs;
  return true;
}

bool read_from_swap(void *kpage, size_t index) {
  bitmap_scan_and_flip(swap_bmap, index, 1, true);
  if (index == BITMAP_ERROR) {
    return false;
  }
  block_sector_t ofs = (block_sector_t) (index * (PGSIZE / BLOCK_SECTOR_SIZE));
  read_from_block(kpage, ofs);
  bitmap_set(swap_bmap, index, true);

  return true;
}