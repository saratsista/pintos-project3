#include <string.h>
#include "frame.h"
#include "page.h"
#include "swap.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/malloc.h"
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
  lock_init (&ft_lock);
  return hash_init (&frame_table, frame_table_hash, frame_less_func, NULL);
}

static bool
load_from_file (struct sup_page_entry *spte, void *kpage);

void *
allocate_page_frame (struct sup_page_entry *spte)
{

  /* Get a page of memory from user_pool */
  void *frame =  palloc_get_page (PAL_USER | PAL_ZERO);
  lock_acquire (&ft_lock);
  if (frame)
  {
    if (!add_frame_to_ft (frame, spte))
      return NULL;
  }
  else
   {
      bool success;
      while ((success = evict_page_frame ()) != true)
	{ 
          lock_release (&ft_lock);
        }
      if (!success)
          PANIC ("No Swap space available!");
      /* allocate new frame */
      frame = palloc_get_page (PAL_USER | PAL_ZERO);
      if (!add_frame_to_ft (frame, spte))
        return NULL;
   }
  lock_release (&ft_lock);
  return frame;
}

bool
add_frame_to_ft (void *frame, struct sup_page_entry *spte)
{
  struct frame_table_entry *fte = calloc (1, sizeof (struct frame_table_entry));
    /* Insert an fte into the frame_table */
    fte->kvaddr = frame; 
    fte->spte = spte;
    fte->t = thread_current ();
    fte->is_pinned = true;
    hash_insert (&frame_table, &fte->elem);

    /* Insert the corresponding element in LRU Cache */
    struct lru_entry *l = malloc (sizeof (struct lru_entry));
    l->kvaddr = frame;
    list_push_front (&lru_cache, &l->lruelem);

    if (spte)
     {
       fte->vaddr = spte->vaddr;
       spte->kvaddr = frame;
     }

    /* Load from the file */
    if (spte && (spte->location == ON_FILE || spte->location == MMAP))
      {
	if (!load_from_file (spte, fte->kvaddr))
          {
            lock_release (&ft_lock);
            free_page_frame (frame);
            free (fte);
            return false;
          }
         spte->is_loaded = true;
	/* update the mapped_files list to reflect the changes */
         if (spte->location == MMAP)
            update_map_table (spte);
       }
     if (spte && spte->location == ON_SWAP)
       {
         load_from_swap (fte);
       }
  return true;
}

void 
free_page_frame (void *kvaddr)
{
  struct frame_table_entry *pfte;
  struct list_elem *e;
  struct lru_entry *l;
  struct hash_elem *h;
  
  if (kvaddr == NULL)
    return;

  if (!lock_held_by_current_thread (&ft_lock))
     lock_acquire (&ft_lock);
  pfte = lookup_page_frame (kvaddr);

  h = hash_delete (&frame_table, &pfte->elem); 

  /*free the frame */
  pagedir_set_dirty (thread_current ()->pagedir, pfte->vaddr, false);
  pagedir_set_accessed (thread_current()->pagedir, pfte->vaddr, false);
  palloc_free_page (kvaddr);

  /* Remove the element from cache */
  for (e = list_begin (&lru_cache); e != list_end (&lru_cache);
       e = list_next (e))
   {
     l = list_entry (e, struct lru_entry, lruelem);
     if (l->kvaddr == kvaddr)
       {
         list_remove (e);
         free (l);
         break;
       }
   }
  if (!lock_held_by_current_thread (&ft_lock))
     lock_release (&ft_lock);
}

struct frame_table_entry *
lookup_page_frame (void *kpage)
{
  struct frame_table_entry fte;
  struct hash_elem *h;

  fte.kvaddr = kpage;
  h = hash_find (&frame_table, &fte.elem);
  /* Move the element to front in the LRU cache */
  if (h != NULL)
     lru_move_to_front (kpage);
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
    return true;
}

void
load_from_swap (struct frame_table_entry *fte)
{
  read_from_swap (fte->spte->swap_index, fte->kvaddr);
  fte->spte->on_swap = false;
  fte->spte->is_loaded = true;
  fte->spte->swap_index = -1;
}

void
lru_move_to_front (void *kvaddr)
{
  struct lru_entry *l;
  struct list_elem *e;

  if (kvaddr == NULL)
     return;

  for (e = list_begin (&lru_cache); e != list_end (&lru_cache);
       e = list_next (e))
   {
     l = list_entry (e, struct lru_entry, lruelem);
     if (kvaddr == l->kvaddr)
      {
        list_remove (e);
        list_push_front (&lru_cache, &l->lruelem);
        break;
      }
   }
}

bool
evict_page_frame (void)
{
  struct list_elem *e = list_pop_back (&lru_cache);
  struct lru_entry *le = list_entry (e, struct lru_entry, lruelem);
  struct frame_table_entry *fte = lookup_page_frame (le->kvaddr);
  struct thread *cur = thread_current ();
  struct sup_page_entry *pspte, spte;
  struct hash_elem *h;

  if (!fte)
    return false;

  if (!pagedir_is_dirty (cur->pagedir, fte->vaddr))
     {
       free_page_frame (fte->kvaddr);
       free (fte);
       free (le);
       return true;
     }
    
  /* write the frame to the swap */
  size_t swap_idx = write_to_swap (le->kvaddr);
  
 /* Indicate the thread which holds this page that it is being
     swapped out */
  spte.vaddr = fte->vaddr;
  h = hash_find (&fte->t->sup_page_table, &spte.elem);
  
  if (h != NULL)
   {
     pspte = hash_entry (h, struct sup_page_entry, elem);
     pspte->location = ON_SWAP;
     pspte->on_swap = true; 
     pspte->swap_index = swap_idx; 
     pspte->is_loaded = false;
     pspte->kvaddr = NULL;
     free_page_frame (le->kvaddr);
     pagedir_clear_page (fte->t->pagedir, pspte->vaddr);
     free (fte);
     free (le);
     return true;
   }
  return false;
}

