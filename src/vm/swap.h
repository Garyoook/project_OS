#ifndef PINTOS_47_SWAP_H
#define PINTOS_47_SWAP_H

#include <stdint.h>
#include <debug.h>
#include "lib/kernel/hash.h"

//struct block *swap_table;

struct swap_entry {
    struct file *file1;
    struct thread *t;
    uint8_t *upage;
    uint8_t *kpage;
    uint8_t *frame;
    struct block *block1;
    struct hash_elem swap_elem;
};

struct hash swap_table;

struct swap_entry *swap_create(uint8_t *upage, struct thread *t, void *frame);

unsigned
swap_hash (const struct hash_elem *e, void *aux UNUSED);

bool
swap_less (const struct hash_elem *a, const struct hash_elem *b,
                   void *aux UNUSED);
#endif //PINTOS_47_SWAP_H




