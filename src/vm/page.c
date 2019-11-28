//
// Created by yg9418 on 23/11/19.
//

#include <stdbool.h>
#include "threads/thread.h"
#include "threads/palloc.h"
#include "page.h"
#include "lib/kernel/list.h"
#include "threads/malloc.h"
#include "kernel/hash.h"
#include "threads/vaddr.h"


unsigned
page_hash (const struct hash_elem *e, void *aux UNUSED)
{
  const struct spt_entry *p = hash_entry (e, struct spt_entry, hash_elem);
  return hash_bytes (&p->upage, sizeof p->upage);
}

/* Returns true if page a precedes page b. */
bool
page_less (const struct hash_elem *a, const struct hash_elem *b,
           void *aux UNUSED)
{
  const struct spt_entry *p1 = hash_entry (a, struct spt_entry, hash_elem);
  const struct spt_entry *p2 = hash_entry (b, struct spt_entry, hash_elem);

  return p1->upage < p2->upage;
}

void sub_page_table_init() {
  hash_init(thread_current()->spt_hash_table, &page_hash, &page_less, NULL);
}


bool page_create(uint32_t *vaddr, struct file *file, enum page_status status, bool writable, off_t offset) {
  struct spt_entry *page = (struct spt_entry *) malloc(sizeof(struct spt_entry));
  if (page == NULL) {
    return false;
  }
  page->upage = vaddr;
  page->status = status;
  page->file = file;
  page->writtable = writable;
  page->offset = offset;
  hash_insert(thread_current()->spt_hash_table, &page->hash_elem);
  return true;
}

struct spt_entry * lookup_page(uint32_t *vaddr) {
  struct spt_entry *page= malloc(sizeof(struct spt_entry));
  page->upage = vaddr;
  struct hash_elem *e = hash_find(thread_current()->spt_hash_table, &page->hash_elem);
  free(page);
  if(e){
    return hash_entry(e,struct spt_entry, hash_elem);
  }
  return NULL;
  }






