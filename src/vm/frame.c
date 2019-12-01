//
// Created by cl10418 on 23/11/19.
//
#include "vm/frame.h"
#include "threads/palloc.h"
#include <debug.h>
#include "lib/kernel/list.h"
#include "threads/thread.h"
#include <string.h>

void frame_evict(void);

struct list frame_table;

void frame_table_init(void)
{
  list_init(&frame_table);
}

struct frame_entry * frame_create(uint32_t *addr)
{
  struct frame_entry *f;

  f = palloc_get_page(PAL_USER | PAL_ZERO);
  if (f == NULL) {
    frame_evict();
    return NULL;
  }

  struct spt_entry *page = page_lookup(addr);
  if (page == NULL) {
    return  NULL;
  }


  memset(f, 0, sizeof *f);
  f->page = addr;
  f->td = thread_current();
//  f->offset = page->offset;
  f->spt = page;

  list_push_back(&frame_table, &f->elem);
  return f;
}

struct frame_entry *frame_lookup(void *addr) {
  struct list_elem *e = list_begin(&frame_table);
  while (e != list_end(&frame_table)) {
    struct frame_entry *frame = list_entry(e, struct frame_entry, elem);
    if (frame->page == addr) {
      return frame;
    }
    e = e->next;
  }
  return NULL;
}

int frame_table_size()
{
  return (int)list_size(&frame_table);
}

void frame_evict(void)
{
  PANIC("GET PAGE FAIL");
}


