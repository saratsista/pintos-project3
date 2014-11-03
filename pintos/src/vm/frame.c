#include <hash.h>
#include "frame.h"
#include "threads/palloc.h"

unsigned
frame_table_hash (const struct hash_elem *h, void *aux UNUSED)
{
  const struct frame_table_entry *fte =
			 hash_entry (h, struct frame_table_entry, elem);

  return hash_bytes (fte->vaddr, sizeof fte->vaddr);
}

bool
frame_less_func (const struct hash_elem *a, const struct hash_elem *b,
		 void *aux UNUSED)
{
  const struct frame_table_entry *fte1 =
			 hash_entry (a, struct frame_table_entry, elem);
  const struct frame_table_entry *fte2 =
			 hash_entry (b, struct frame_table_entry, elem);

  return fte1->vaddr < fte2->vaddr;
}

bool
init_frame_table ()
{
  return  hash_init (&frame_table, frame_table_hash, frame_less_func, NULL);
}

void *
allocate_page_frame (void *vaddr)
{
  void *frame =  palloc_get_page (PAL_USER);
  if (frame)
  {
    struct frame_table_entry fte;
    fte.vaddr = vaddr;
    fte.kvaddr = frame; 
    hash_insert (&frame_table, &fte.elem);
  }
  else
  {
    PANIC ("Cannot allocate frame from user pool");
  }
}
