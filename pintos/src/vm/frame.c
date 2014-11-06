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

  return hash_bytes (&fte->vaddr, sizeof fte->vaddr);
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

static bool
install_page_frame (void *uaddr, void *frame, bool writable);

bool
allocate_page_frame (struct sup_page_entry *spte)
{
  struct frame_table_entry *fte = malloc (sizeof (struct frame_table_entry));

  /* Get a page of memory from user_pool */
  void *frame =  palloc_get_page (PAL_USER);

  if (frame)
  {
    fte->vaddr = spte->vaddr;
    fte->kvaddr = frame; 
    fte->spte = spte;
    hash_insert (&frame_table, &fte->elem);

    spte->kvaddr = frame;
    /* Load this page */   
    lock_acquire (&filesys_lock);
    spte->file = file_open (spte->inode);
    if (file_read_at (spte->file, frame, spte->read_bytes, spte->file_off) 
		!= (int)spte->read_bytes)
    {
      free_page_frame (frame);
      free (fte);
      lock_release (&filesys_lock);
      return false;
    }
    lock_release (&filesys_lock);

    memset (frame + spte->read_bytes, 0, spte->zero_bytes);

   /* Add the page to process' address space */
    if (!install_page_frame (spte->vaddr, frame, spte->writable))
    {
      free_page_frame (frame);
      free (fte);
      return false;
    }
    spte->is_loaded = true;
  }
  else
  {
    PANIC ("Cannot allocate frame from user pool");
  }
  return true;
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
lookup_page_frame (void *vaddr)
{
  struct frame_table_entry fte;
  struct hash_elem *h;

  fte.vaddr = vaddr;
  h = hash_find (&frame_table, &fte.elem);
  return (h == NULL ? NULL : hash_entry (h, struct frame_table_entry, elem));
} 

static bool
install_page_frame (void *uaddr, void *frame, bool writable)
{
  struct thread *t = thread_current ();
  
  return (pagedir_get_page (t->pagedir, uaddr) == NULL
	  && pagedir_set_page (t->pagedir, uaddr, frame, writable));
}
