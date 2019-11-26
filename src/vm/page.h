//
// Created by yg9418 on 23/11/19.
//

#ifndef PINTOS_47_PAGE_H
#define PINTOS_47_PAGE_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/hash.h"
#include "kernel/list.h"
#include "filesys/off_t.h"


struct spt_entry {
  uint32_t *page;
  bool in_file_sys;
  bool in_swap_slot;
  bool all_zero_page;
  struct list_elem sub_page_elem;
  struct hash_elem hash_elem;
  struct file *file;
  off_t offset;
  bool writtable;
};

void sub_page_table_init();
void page_create(uint32_t *vaddr);
struct spt_entry * lookup_page(const uint32_t *vaddr);

#endif //PINTOS_47_PAGE_H
