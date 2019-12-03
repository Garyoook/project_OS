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

#include "page.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "kernel/hash.h"

#include "threads/malloc.h"

struct spage{
  struct file *file1;
  off_t offset;
  uint8_t *upage;
  uint8_t *kpage;
  uint32_t read_bytes;
  uint32_t zero_bytes;
  bool writable;
  struct hash_elem pelem;
  bool has_load_in;
};

unsigned
page_hash (const struct hash_elem *e, void *aux);

bool
page_less (const struct hash_elem *a, const struct hash_elem *b,
           void *aux);

void sub_page_table_init();
bool create_spage(struct file *file, off_t ofs, uint8_t *upage,
                  uint32_t read_bytes, uint32_t zero_bytes, bool writable);
struct spage* lookup_spage(uint8_t *upage);
void spage_destroy(uint8_t* upage);
#endif //PINTOS_47_PAGE_H
