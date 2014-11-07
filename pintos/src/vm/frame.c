#include <string.h>
#include "frame.h"
#include "page.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "userprog/syscall.h"
#include "userprog/pagedir.h"

unsigned
frame_table_hash (const struct hash_elem *h, void *aux UNUSED)
{
  const struct frame_table_entry *fte =
			 hash_entry (h, struct frame_table_entry, elem);

  return hash_bytes (&fte->kvaddr, sizeof fte->kvaddr);
}

bool
frame_less_func (const struct hash_elem *a, const struct hash_elem *b,
		 void *aux UNUSED)
{
  const struct frame_table_entry *fte1 =
			 hash_entry (a, struct frame_table_entry, elem);
  const struct frame_table_entry *fte2 =
			 hash_entry (b, struct frame_table_entry, elem);

  return fte1->kvaddr < fte2->kvaddr;
}

bool
init_frame_table ()
{
  return  hash_init (&frame_table, frame_table_hash, frame_less_func, NULL);
}

static bool
load_from_file (struct sup_page_entry *spte, void *kpage);

void *
allocate_page_frame (struct sup_page_entry *spte)
{
  struct frame_table_entry *fte = calloc (1, sizeof (struct frame_table_entry));

  /* Get a page of memory from user_pool */
  void *frame =  palloc_get_page (PAL_USER | PAL_ZERO);

  if (frame)
  {
    fte->kvaddr = frame; 
    fte->spte = spte;
    hash_insert (&frame_table, &fte->elem);

    if (spte && spte->location == ON_FILE)
      {
    	fte->vaddr = spte->vaddr;
	if (!load_from_file (spte, fte->kvaddr))
          {
            free_page_frame (frame);
            free (fte);
            return NULL;
          }
       }
   }
  else
   {
      PANIC ("Cannot allocate frame from user pool");
   }
  return frame;
}

void 
free_page_frame (void *vaddr)
{
  struct frame_table_entry fte;
  
  fte.vaddr = vaddr;
  hash_delete (&frame_table, &fte.elem); 
  palloc_free_page (vaddr);
}

struct frame_table_entry *
lookup_page_frame (void *kpage)
{
  struct frame_table_entry fte;
  struct hash_elem *h;

  fte.kvaddr = kpage;
  h = hash_find (&frame_table, &fte.elem);
  return (h == NULL ? NULL : hash_entry (h, struct frame_table_entry, elem));
} 

bool
install_page_frame (void *uaddr, void *frame, bool writable)
{
  struct thread *t = thread_current ();

  return (pagedir_get_page (t->pagedir, uaddr) == NULL
    && pagedir_set_page (t->pagedir, uaddr, frame, writable));
}

static bool
load_from_file (struct sup_page_entry *spte, void *frame)
{
    spte->kvaddr = frame;
    /* Load this page */   
    spte->file = file_open (spte->inode);
    if (file_read_at (spte->file, frame, spte->read_bytes, spte->file_off) 
		!= (int)spte->read_bytes)
    {
      return false;
    }

    memset (frame + spte->read_bytes, 0, spte->zero_bytes);

   /* Add the page to process' address space */
    if (!install_page_frame (spte->vaddr, frame, spte->writable))
    {
      return false;
    }
    spte->is_loaded = true;
    return true;
}
