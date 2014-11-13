#include <string.h>
#include "filesys/file.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "page.h"


unsigned
sup_page_hash (const struct hash_elem *h, void *aux UNUSED)
{
  const struct sup_page_entry *spte =
                         hash_entry (h, struct sup_page_entry, elem);

  return hash_bytes (&spte->vaddr, sizeof spte->vaddr);
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

/* Function which deletes and frees each element in the hash table */
void
sup_page_destroy (struct hash_elem *h, void *aux UNUSED)
{
  struct frame_table_entry *fte;
  struct sup_page_entry *spte =
			hash_entry (h, struct sup_page_entry, elem);
  if (spte->is_loaded)
   {
     fte = lookup_page_frame (spte->kvaddr);
     if (fte)
     {
       free_page_frame (fte->kvaddr);
       free (fte);
     }
     pagedir_clear_page (thread_current ()->pagedir, spte->vaddr);
   }
   free (spte);
}

struct sup_page_entry *
add_to_spt (struct file *file, uint32_t off, uint8_t *upage, bool writable,
		  size_t page_read_bytes, size_t page_zero_bytes, enum data_location location)
{
  struct thread *cur = thread_current ();
  struct sup_page_entry *spte = malloc (sizeof (struct sup_page_entry));
  struct hash_elem *h;

  if (spte == NULL)
    return false;

  spte->location = location;
  spte->vaddr = upage;
  spte->kvaddr = NULL;
  spte->file = file;
  spte->inode = file_get_inode (file); 
  spte->file_off = off;
  spte->writable = writable;
  spte->read_bytes = page_read_bytes; 
  spte->zero_bytes = page_zero_bytes;
  spte->is_loaded = false;
  spte->mapid = -1;
  spte->on_swap = false;
  
  h = hash_insert (&cur->sup_page_table, &spte->elem);

  if ( h != NULL)
     return NULL;
  return spte;
}

/* Lookup an element in sup_page_table with key VADDR */
struct sup_page_entry *
lookup_sup_page (void *vaddr)
{
  struct sup_page_entry spte, *pspte; 
  struct hash_elem *h;
  struct thread *cur = thread_current ();

  spte.vaddr = pg_round_down (vaddr);
  h = hash_find (&cur->sup_page_table, &spte.elem);
  if (h != NULL)
   {
     pspte = hash_entry (h, struct sup_page_entry, elem); 
     /* If a page frame is already allocated, move it to front
        in the LRU cache */ 
     if (pspte->is_loaded)
     	lru_move_to_front (pspte->kvaddr);
     return pspte;
   }
   return NULL;
}

void
free_spt_entry (struct sup_page_entry *spte)
{
  struct thread *cur = thread_current ();
  struct hash_elem *h;
  struct frame_table_entry *fte;
  fte = hash_entry (h, struct frame_table_entry, elem);

  if (spte)
   {
     h = hash_delete (&cur->sup_page_table, &spte->elem);
     if (spte->kvaddr)
      {
        free_page_frame (spte->kvaddr);
        free (fte);
        pagedir_clear_page (cur->pagedir, spte->vaddr);
      }
     free (spte);
   }
}

/* Destroy the sup_page_table */
void
spt_destroy (struct hash *spt)
{
  hash_destroy (spt, sup_page_destroy);
}

/* Update the mmapped_files table */
void
update_map_table (struct sup_page_entry *spte)
{
  struct thread *cur = thread_current ();
  struct list *map_list = &cur->mapped_files;
  struct map_page *mpage; 
  struct list_elem *e;

  for (e = list_begin (map_list); e != list_end (map_list);
       e = list_next (e))
    {
      mpage = list_entry (e, struct map_page, map_elem);
      if (mpage->spte->vaddr == spte->vaddr)
        {
          mpage->spte = spte;
          list_remove (e);
          break;
        }
    }
}
