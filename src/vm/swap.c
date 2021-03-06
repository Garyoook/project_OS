

#include "threads/vaddr.h"
#include "threads/interrupt.h"
#include "kernel/list.h"
#include "kernel/bitmap.h"
#include "swap.h"
#include "devices/block.h"
#include "page.h"

#define SECTOR_COUNT (PGSIZE/BLOCK_SECTOR_SIZE)


size_t get_free_slot(size_t size);
struct lock swap_lock;
struct lock block_lock;

void init_swap_block(){
  b = block_get_role(BLOCK_SWAP);
  bmap = bitmap_create(block_size(b) * 4);
  bitmap_set_all(bmap, 0);
  lock_init(&swap_lock);
  lock_init(&block_lock);
}

block_sector_t write_to_swap(void* page) {
  lock_acquire(&swap_lock);
  uint32_t start = bitmap_scan(bmap, 0, SECTOR_COUNT, 0);
  bitmap_set_multiple(bmap, start, SECTOR_COUNT, 1);
  lock_release(&swap_lock);
  for (int i = 0; i < SECTOR_COUNT; i++) {
    lock_acquire(&block_lock);
    block_write(b, (block_sector_t) start + i, page + i * BLOCK_SECTOR_SIZE);
    lock_release(&block_lock);
  }
  return (block_sector_t) start;
}

void read_from_swap(void* uspage,void* kepage) {
  struct swap_entry *swapEntry = lookup_swap(uspage);
  size_t  start = swapEntry->blockSector;
  lock_acquire(&swap_lock);
  bitmap_set_multiple(bmap, start, SECTOR_COUNT, 0);

  list_remove(&swapEntry->s_elem);
  free(swapEntry);
  lock_release(&swap_lock);
  for (int i = 0; i < SECTOR_COUNT; i++){
    lock_acquire(&block_lock);
    block_read(b, (block_sector_t) start + i, kepage + i * BLOCK_SECTOR_SIZE);
    lock_release(&block_lock);
  }
}

struct swap_entry* lookup_swap(void* upage) {
    struct list_elem *e = list_begin(&swap_table);
    while (e != list_end(&swap_table)){
      struct swap_entry *this_swap = list_entry(e, struct swap_entry, s_elem);
      if (this_swap->uspage == upage
          && this_swap->t_blongs_to == thread_current()) {
        return this_swap;
      }
      e = e->next;
    }
    return NULL;
}


