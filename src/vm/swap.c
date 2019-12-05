

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

block_sector_t write_to_swap(void* page, struct swap_entry* swapEntry){

//  char hello[10] = "nihao";
//  block_write(b, 0, hello);
//  char nihao[10];
//  block_read(b, 0, nihao);
//  printf("%lalalalal--------%s", nihao);
  size_t start = get_free_slot(sizeof(page) / PGSIZE);
  bitmap_set_multiple(bmap, start, SECTORS_PER_PAGE, 1);
//  swapEntry->blockSector = start;
//block_print_stats();
//  printf("qqq: %d\n", start);
  for (int i = 0; i < SECTORS_PER_PAGE; i++)
    block_write(b, (block_sector_t) start+i, page+ i * (PGSIZE/SECTORS_PER_PAGE) );

  return (block_sector_t) start;
}

void read_from_swap(void* uspage,void* kepage) {

//  printf("%xaaaaaaaaa\n", uspage);
  size_t  start = lookup_swap(uspage)->blockSector;
  bitmap_set_multiple(bmap, start, SECTORS_PER_PAGE, 0);
//  printf("%xhelloaaaaaaaaaa\n", lookup_swap(uspage)->blockSector);

  list_remove(&lookup_swap(uspage)->s_elem);
  for (int i = 0; i < SECTORS_PER_PAGE; i++){
//    printf("%d reads\n", i);
    block_read(b, (block_sector_t) start+i, uspage + i * (PGSIZE/SECTORS_PER_PAGE) );
  }
//  printf("fnish read\n");

}

size_t get_free_slot(size_t size){
  return bitmap_scan(bmap, 0, SECTORS_PER_PAGE, 0);
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

