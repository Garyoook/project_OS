#ifndef PINTOS_47_SWAP_H
#define PINTOS_47_SWAP_H

#include "stdbool.h"
#include "stddef.h"

void swap_init();
size_t write_to_block(void* page);
void read_from_block(void* page, size_t index);

#endif //PINTOS_47_SWAP_H




