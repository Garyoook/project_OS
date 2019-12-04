#include "threads/malloc.h"
#include "swap.h"
#include "devices/block.h"


unsigned
swap_hash (const struct hash_elem *e, void *aux UNUSED) {
  const struct swap_entry *s = hash_entry(e, struct swap_entry, swap_elem);
  return hash_bytes(&s->upage, sizeof(s->upage));
}

bool
swap_less (const struct hash_elem *a, const struct hash_elem *b,
           void *aux UNUSED)
{
  const struct swap_entry *s1 = hash_entry (a, struct swap_entry, swap_elem);
  const struct swap_entry *s2 = hash_entry (b, struct swap_entry, swap_elem);

  return s1->upage < s2->upage;
}

struct swap_entry* swap_create(uint8_t *upage, struct thread *t, void *frame) {
  struct swap_entry *new_swap = malloc(sizeof(struct swap_entry));
  if (new_swap != NULL) {
    new_swap->upage = upage;
    new_swap->t = t;
    new_swap->frame = frame;
    new_swap->block1 =
    hash_insert(&swap_table, &new_swap->swap_elem);
    return new_swap;
  }
  return NULL;
}
//struct block *swap_create() {
//  swap_table = block_get_role(BLOCK_SWAP);
//}