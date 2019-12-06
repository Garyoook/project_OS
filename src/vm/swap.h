#ifndef PINTOS_47_SWAP_H
#define PINTOS_47_SWAP_H

#include "threads/thread.h"
#include "kernel/list.h"
#include "kernel/bitmap.h"
#include "devices/block.h"
#include "page.h"
struct block* b;
struct bitmap* bmap;

void init_swap_block();
void read_from_swap(void* upage, void* kpage);
struct list swap_table;
struct swap_entry{
  void* uspage;
  struct thread *t_blongs_to;
  block_sector_t blockSector;
  struct list_elem s_elem;
};
block_sector_t write_to_swap(void* page, struct swap_entry* swapEntry);
struct swap_entry* lookup_swap(void* upage);
void swap_debug_dump(void);

void swap_debug_dump(void);

#endif //PINTOS_47_SWAP_H




