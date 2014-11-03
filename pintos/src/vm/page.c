#include <hash.h>
#include "page.h"


unsigned
sup_page_hash (const struct hash_elem *h, void *aux UNUSED)
{
  const struct sup_page_entry *spte =
                         hash_entry (h, struct sup_page_entry, elem);

  return hash_bytes (spte->vaddr, sizeof spte->vaddr);
}

bool
sup_page_less (const struct hash_elem *a, const struct hash_elem *b,
                 void *aux UNUSED)
{
  const struct sup_page_entry *spte1 =  
                          hash_entry (a, struct sup_page_entry, elem);
  const struct sup_page_entry *spte2 = 
                          hash_entry (b, struct sup_page_entry, elem);

  return spte1->vaddr < spte2->vaddr;
}

void *add_to_spt (void *vaddr)
{
}

void free_spt_entry (void *vaddr)
{
}

