//
// Created by wz5918 on 30/11/19.
//

#ifndef PINTOS_47_PAGE_H
#define PINTOS_47_PAGE_H

#include <stdbool.h>
#include <stdint.h>
#include "filesys/off_t.h"
#include "filesys/file.h"
#include "lib/kernel/hash.h"
#include "list.h"
#include "threads/palloc.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "kernel/hash.h"

#include "threads/malloc.h"

struct spage{
  struct file *file_sp;
  off_t offset;
  uint8_t *upage;
  uint8_t *kpage;
  uint32_t read_bytes;
  uint32_t zero_bytes;
  bool writable;
  struct hash_elem pelem;
  bool for_lazy_load;
  bool  has_load_in;
  size_t position_in_swap;
  bool in_swap_table;
  int md;
};

void spage_init();

unsigned page_hash (const struct hash_elem *e, void *aux);

bool page_less (const struct hash_elem *a, const struct hash_elem *b,
           void *aux);

struct spage *create_spage(struct file *file, off_t ofs, uint8_t *upage,
                  uint32_t read_bytes, uint32_t zero_bytes, bool writable);
struct spage* lookup_spage(uint8_t *upage);
void spage_destroy(uint8_t* upage);
#endif //PINTOS_47_PAGE_H
