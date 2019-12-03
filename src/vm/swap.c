#include "swap.h"
#include "devices/block.h"

void swap_init() {
  swap_index = 0;
  swap_block = block_get_role(BLOCK_SWAP);
}

void read_from_block(void* frame, int index) {
  for(int i = 0; i < 8; i++) {
    block_read(swap_block, (block_sector_t) (index + i), frame + (i * BLOCK_SECTOR_SIZE));
  }
}

void write_to_block(void* frame, int index) {
  for(int i = 0; i < 8; i++) {
    block_write(swap_block, (block_sector_t) (index + i), frame + (i * BLOCK_SECTOR_SIZE));
  }
}