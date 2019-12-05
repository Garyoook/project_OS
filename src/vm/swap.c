

#include "threads/vaddr.h"
#include "threads/interrupt.h"
#include "kernel/list.h"
#include "kernel/bitmap.h"
#include "swap.h"
#include "devices/block.h"
#include "page.h"

#define SECTORS_PER_PAGE 8

size_t get_free_slot(size_t size);

void init_swap_block(){
  b = block_get_role(BLOCK_SWAP);
  bmap = bitmap_create(block_size(b) * 4);
  bitmap_set_all(bmap, 0);
}

block_sector_t write_to_swap(void* something, struct swap_entry* swapEntry){
  size_t start = get_free_slot(sizeof(something) / PGSIZE);
  bitmap_set_multiple(bmap, start, 8, 1);
//  swapEntry->blockSector = start;
//block_print_stats();
//  printf("qqq: %d\n", start);
  for (int i = 0; i < 8; i++)
    block_write(b, (block_sector_t) start+i, something+ i * (PGSIZE/8) );

  return (block_sector_t) start;
}

void read_from_swap(void* uspage,void* kepage) {

//  printf("%xaaaaaaaaa\n", uspage);
  size_t  start = lookup_swap(uspage)->blockSector;
  bitmap_set_multiple(bmap, start, 8, 0);
//  printf("%xhelloaaaaaaaaaa\n", lookup_swap(uspage)->blockSector);

  list_remove(&lookup_swap(uspage)->s_elem);
  for (int i = 0; i < 8; i++){
//    printf("%d reads\n", i);
    block_read(b, (block_sector_t) start+i, kepage + i * (PGSIZE/8) );
  }
//  printf("fnish read\n");

}

size_t get_free_slot(size_t size){
  return bitmap_scan(bmap, 0, 8, 0);
}

struct swap_entry* lookup_swap(void* upage) {
    struct list_elem *e = list_begin(&swap_table);
    while (e != list_end(&swap_table)){
      struct swap_entry *this_swap = list_entry(e, struct swap_entry, s_elem);
      if (this_swap->uspage == upage && this_swap->t_blongs_to == thread_current()) {
        return this_swap;
      }
      e = e->next;
    }
    return NULL;
}

