#include <hash.h>
#include "threads/malloc.h"
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

bool
add_to_spt (struct file *file, uint8_t *upage, bool writable,
		  size_t page_read_bytes, size_t page_zero_bytes)
{
  struct sup_page_entry *spte = malloc (sizeof (struct sup_page_entry));
  struct hash_elem *h;

  if (spte == NULL)
    return false;

  spte->location = ON_FILE;
  spte->vaddr = upage;
  spte->kvaddr = NULL;
  spte->file = file;
  spte->writable = writable;
  spte->read_bytes = page_read_bytes; 
  spte->zero_bytes = page_zero_bytes;
  
  h = hash_insert (&thread_current ()->sup_page_table, &spte->elem);
  if ( h != NULL)
    return false;
  return true;
}

void free_spt_entry (struct hash_elem *h)
{
  hash_delete (&thread_current ()->sup_page_table, h); 
}

