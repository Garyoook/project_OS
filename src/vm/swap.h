#ifndef PINTOS_47_SWAP_H
#define PINTOS_47_SWAP_H

void write_to_block(void* frame, int index);
void read_from_block(void* frame, int index);
struct bitmap *swap_bmap;
int swap_index;

#endif //PINTOS_47_SWAP_H




