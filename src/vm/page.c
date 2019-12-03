
#include "page.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <kernel/hash.h>
#include "userprog/syscall.h"
#include "userprog/pagedir.h"
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
  new_page->has_load_in = false;
  hash_insert(&thread_current()->spage_table, &new_page->pelem);
 // printf("W%d\n", file_tell(file));


  if (!writable) {
    struct hash_iterator i;
    struct thread *t = thread_current();
    hash_first(&i, &t->spage_table);

    while (hash_next(&i)) {
      struct spage *sp = hash_entry (hash_cur(&i), struct spage, pelem);
      if (sp == NULL)
        break;
      if (file == sp->file1 && !sp->writable) {
        new_page->kpage = sp->kpage;

        if (new_page->kpage != NULL) {
          bool install_page = (pagedir_get_page(t->pagedir, upage) == NULL
                               && pagedir_set_page(t->pagedir, upage, new_page->kpage, writable));
          if (!install_page)
            exit(-1);
        }
        break;
      }
    }
  }
  file_seek(file, 0);
  return true;
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