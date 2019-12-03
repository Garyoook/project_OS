#ifndef PINTOS_47_SWAP_H
#define PINTOS_47_SWAP_H
#include "kernel/list.h"
#include "kernel/bitmap.h"
#include "devices/block.h"
#include "page.h"
struct block* b;
struct bitmap* bmap;
void write_to_swap(void* something);
void init_swap_block();
void read_from_swap(void* something);
size_t get_free_slot(size_t size);

#endif //PINTOS_47_SWAP_H




