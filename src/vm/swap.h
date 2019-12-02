#ifndef PINTOS_47_SWAP_H
#define PINTOS_47_SWAP_H

static struct block *swap_block;

void block_init();
void read_from_block(void *frame, int index);
void write_to_block(void *frame, int index);

#endif //PINTOS_47_SWAP_H




