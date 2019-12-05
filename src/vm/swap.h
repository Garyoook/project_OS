#ifndef PINTOS_47_SWAP_H
#define PINTOS_47_SWAP_H

#include "stdbool.h"
#include "stddef.h"
#include "threads/synch.h"

void swap_init();
void write_to_block(void* frame, int index);
void read_from_block(void* frame, int index);
bool write_to_swap(void *kpage);
size_t read_from_swap(void *kpage, size_t start);
struct bitmap *swap_bmap;
int swap_index_global;
static struct block *swap_block;
struct lock swap_lock;

#endif //PINTOS_47_SWAP_H




