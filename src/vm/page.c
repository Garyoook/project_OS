//
// Created by yg9418 on 23/11/19.
//

#include <stdbool.h>
#include "threads/palloc.h"
#include "page.h"
#include "lib/kernel/list.h"
#include "threads/malloc.h"

struct list sub_page_table;

void sub_page_table_init() {
  list_init(&sub_page_table);
}

void page_create(uint32_t *vaddr) {
  struct sub_page_table_entry *page = malloc(sizeof(struct sub_page_table_entry));
  if (page == NULL) {
    return;
  }
  page->in_file_sys = true;
  page->in_swap_slot = false;
  page->all_zero_page = false;
  page->vaddr = vaddr;

  list_push_back(&sub_page_table, &page->sub_page_elem);
}

struct sub_page_table_entry * lookup_page(const uint32_t *vaddr) {
  struct list_elem * e = list_pop_front(&sub_page_table);
  while (e != list_end(&sub_page_table)) {
    struct sub_page_table_entry *page = list_entry(e, struct sub_page_table_entry, sub_page_elem);
    if(page->vaddr == vaddr) {
      return page;
    }

    e = e->next;
  }
}



