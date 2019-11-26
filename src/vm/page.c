//
// Created by yg9418 on 23/11/19.
//

#include <stdbool.h>
#include "threads/palloc.h"
#include "page.h"
#include "lib/kernel/list.h"
#include "threads/malloc.h"

struct list sup_page_table;

void sub_page_table_init() {
  list_init(&sup_page_table);
}

void page_create(uint32_t *vaddr) {
  struct spt_entry *page = malloc(sizeof(struct spt_entry));
  if (page == NULL) {
    return;
  }
  page->in_file_sys = true;
  page->in_swap_slot = false;
  page->all_zero_page = false;
  page->page = vaddr;

  list_push_back(&sup_page_table, &page->sub_page_elem);
}

struct spt_entry * lookup_page(const uint32_t *vaddr) {
  struct list_elem * e = list_pop_front(&sup_page_table);
  while (e != list_end(&sup_page_table)) {
    struct spt_entry *page = list_entry(e, struct sub_page_table_entry, sub_page_elem);
    if(page->page == vaddr) {
      return page;
    }

    e = e->next;
  }
}



