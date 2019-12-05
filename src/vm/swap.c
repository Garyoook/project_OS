#include "swap.h"
#include "devices/block.h"
#include "kernel/bitmap.h"
#include "threads/vaddr.h"

static struct bitmap *swap_bmap;
static struct block *swap_block;

static const int SECTORS_PRE_PAGE = PGSIZE / BLOCK_SECTOR_SIZE;

void swap_init() {
  swap_block = block_get_role(BLOCK_SWAP);
  swap_bmap = bitmap_create(block_size(swap_block)/SECTORS_PRE_PAGE); /* All false in bits */
}

void read_from_block(void* page, size_t index) {
  for(int i = 0; i < SECTORS_PRE_PAGE; i++) {
    block_read(swap_block, (block_sector_t) (index * SECTORS_PRE_PAGE + i), page + (i * BLOCK_SECTOR_SIZE));
  }
  bitmap_set(swap_bmap, index, false);
}

size_t write_to_block(void* page) {
  size_t index = bitmap_scan(swap_bmap, 0, 1, false);
  for(int i = 0; i < SECTORS_PRE_PAGE; i++) {
    block_write(swap_block, (block_sector_t) (index * SECTORS_PRE_PAGE + i), page + (i * BLOCK_SECTOR_SIZE));
  }
  bitmap_set(swap_bmap, index, true);
  return index;
}