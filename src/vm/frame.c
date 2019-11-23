//
// Created by cl10418 on 23/11/19.
//
#include <palloc.h>
#include <debug.h>
#include "lib/kernel/list.h"
#include <thread.h>
#include <string.h>

void frame_evict(void)

struct list *frame_table;
struct frame_entry
{
    uint32_t *page;
    struct thread *td;
    struct list_elem elem;
};

void frame_table_init(void)
{
  list_init(frame_table);
}

void frame_create(uint32_t *page)
{
  struct frame_entry *f;

  f = palloc_get_page(PAL_USER);
  if (f == NULL) {
    frame_evict();
    return;
  }

  memset(f, 0, sizeof *f);
  f->page = page;
  f->td = thread_current();

  list_push_back(frame_table, &f->elem);
}

int frame_table_size()
{
  return (int)list_size(frame_table);
}

void frame_evict(void)
{
  PANIC("GET PAGE FAIL");
}


