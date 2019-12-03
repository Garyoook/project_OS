#ifndef PINTOS_47_SWAP_H
#define PINTOS_47_SWAP_H

#include "stdbool.h"
#include "stddef.h"

void write_to_block(void* frame, int index);
void read_from_block(void* frame, int index);
bool write_to_swap(void *kpage);
bool read_from_swap(void *kpage, size_t index);
struct bitmap *swap_bmap;
int swap_index;

#endif //PINTOS_47_SWAP_H




