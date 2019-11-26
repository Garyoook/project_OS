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

enum page_status {
  IN_FRAME_TABLE,
  IN_SWAP_SLOT,
  IN_FILESYS,
  ALL_ZERO
};

struct spt_entry {
  uint32_t *upage;
  enum page_status status;
  struct file *file;
  off_t offset;
  bool writtable;
  struct hash_elem hash_elem;
  struct list_elem sub_page_elem;
};



void sub_page_table_init();
bool page_create(uint32_t *vaddr, struct file *file, enum page_status status, bool writable, off_t offset);
struct spt_entry * lookup_page(const uint32_t *vaddr);

#endif //PINTOS_47_PAGE_H
