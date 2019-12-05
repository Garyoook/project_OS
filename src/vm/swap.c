

#include "threads/vaddr.h"
#include "threads/interrupt.h"
#include "kernel/list.h"
#include "kernel/bitmap.h"
#include "swap.h"
#include "devices/block.h"
#include "page.h"

#define SECTOR_COUNT (PGSIZE/BLOCK_SECTOR_SIZE)

size_t get_free_slot(size_t size);

void init_swap_block(){
  b = block_get_role(BLOCK_SWAP);
  bmap = bitmap_create(block_size(b) * 8);
  bitmap_set_all(bmap, 0);
}

block_sector_t write_to_swap(void* page, struct swap_entry* swapEntry){

//  char hello[10] = "nihao";
//  block_write(b, 0, hello);
//  char nihao[10];
//  block_read(b, 0, nihao);
//  printf("%lalalalal--------%s", nihao);

  size_t start = bitmap_scan_and_flip(bmap, 0, SECTOR_COUNT, 0);
  if (start == 904) printf("GHGHGHGH\n");
//  bitmap_set_multiple(bmap, start, SECTOR_COUNT, 1);
//  swapEntry->blockSector = start;
//block_print_stats();
//  printf("qqq: %d\n", start);
  for (int i = 0; i < SECTOR_COUNT; i++)
    block_write(b, (block_sector_t) start+i, page + i * BLOCK_SECTOR_SIZE );

  return (block_sector_t) start;
}

void read_from_swap(void* uspage,void* kepage) {
//  printf("%xaaaaaaaaa\n", uspage);
  size_t  start = lookup_swap(uspage)->blockSector;
  if (start == 904) printf(" %p\n", uspage);
  bitmap_set_multiple(bmap, start, SECTOR_COUNT, 0);
//  printf("%xhelloaaaaaaaaaa\n", lookup_swap(uspage)->blockSector);

  struct swap_entry *se = lookup_swap(uspage);
  if (se == NULL) {
    return;
  }
  list_remove(&se->s_elem);
  for (int i = 0; i < SECTOR_COUNT; i++){
//    printf("%d reads\n", i);
    block_read(b, (block_sector_t) start+i, kepage + i * BLOCK_SECTOR_SIZE );
  }
//  printf("fnish read\n");

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

void swap_debug_dump(void)
{
  printf("==========SWAP DUMP=================\n");
  for (struct list_elem *e = list_begin(&swap_table)
          ; e != list_end(&swap_table)
          ; e = list_next(e))
  {
    struct swap_entry *se = list_entry(e, struct swap_entry, s_elem);
    printf("vaddr -> %p, owned by thread %s, blocksector = %d\n", se->uspage, se->t_blongs_to, se->blockSector);
  }
}

