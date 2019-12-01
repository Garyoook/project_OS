
#include "page.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <kernel/hash.h>
#include "threads/thread.h"
#include "threads/malloc.h"



unsigned
page_hash (const struct hash_elem *e, void *aux UNUSED)
{
  const struct spage *p = hash_entry (e, struct spage, pelem);
  return hash_bytes (&p->upage, sizeof p->upage);
}

/* Returns true if page a precedes page b. */
bool
page_less (const struct hash_elem *a, const struct hash_elem *b,
           void *aux UNUSED)
{
  const struct spage *p1 = hash_entry (a, struct spage, pelem);
  const struct spage *p2 = hash_entry (b, struct spage, pelem);

  return p1->upage < p2->upage;
}


bool create_spage(struct file *file, off_t ofs, uint8_t *upage,
             uint32_t read_bytes, uint32_t zero_bytes, bool writable){
  struct spage *new_page = malloc(sizeof(struct spage));
  if (new_page == NULL) {
    return false;
  }
  new_page->file1 = file;
  new_page->offset = ofs;
  new_page->upage = upage;
  new_page->read_bytes = read_bytes;
  new_page->zero_bytes = zero_bytes;
  new_page->writable = writable;
  new_page->for_lazy_load = true;
  hash_insert(&thread_current()->spage_table, &new_page->pelem);
  bool something = *upage;
//  *upage;
//  printf("PPPPPPPPPPPPPPPPPage addr remembered: %u\n", (uint32_t) *upage);
  return (bool) something;
}

struct spage * lookup_spage(uint8_t* upage) {
  struct spage page;
  page.upage = upage;
  struct hash_elem *e = hash_find(&thread_current()->spage_table, &page.pelem);
  return e != NULL ? hash_entry(e, struct spage, pelem): NULL;
}

void spage_destroy(uint8_t* upage) {
  struct spage *page = lookup_spage(upage);
  if (!page) {
    return;
  }
  hash_delete(&thread_current()->spage_table, &page->pelem);
  free(page);

}