#ifndef PINTOS_47_SWAP_H
#define PINTOS_47_SWAP_H
#include "kernel/list.h"
#include "kernel/bitmap.h"
#include "devices/block.h"
#include "page.h"
#include "frame.h"

struct swap {
    struct frame *frame1;
    size_t position;
    struct spage *page;
    struct list_elem s_elem;
};

struct list swap_table;
struct block* b;
struct bitmap* bmap;
void write_to_swap(struct frame* something);
void init_swap_block();
void read_from_swap(struct frame* something);
size_t get_free_slot(size_t size);

#endif //PINTOS_47_SWAP_H




