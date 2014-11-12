#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <hash.h>
#include <list.h>
#include "threads/thread.h"
#include "threads/synch.h"

/* Frame Table: Frame table is a global data structure and 
   is common for all processes.*/
struct hash frame_table;

/* Lock for the frame_table */
struct lock ft_lock;

/* Structure for each entry in the Frame Table
   The key is the kernel virtual address KVADDR */
struct frame_table_entry
{
  void *vaddr;		  /* User virtual address */
  void *kvaddr;		 /* Kernel virtual address for vaddr */
  struct thread *t;	 /* Thread this frame belongs to */
  struct sup_page_entry *spte; /* Corresponding page table entry */
  struct hash_elem elem;
};

/* LRU cache for eviction */
struct list lru_cache;

struct lru_entry
{
  void *kvaddr;
  struct list_elem lruelem;
};

/* Initialize the frame_table */
bool init_frame_table (void);

/* A function that calculates hash for each frame_table_entry */
hash_hash_func frame_table_hash;

/* Returns true if frame_table_entry A precedes frame_table_entry B */
hash_less_func frame_less_func;

/* Allocate a page frame for VADDR and install the
   mapping in the frame_table */
void *allocate_page_frame (struct sup_page_entry *);

bool
install_page_frame (void *uaddr, void *frame, bool writable);

/* Free a page frame KVADDR from the frame table */
void free_page_frame (void *kvaddr);

/*Function to lookup a page frame with VAADR as its key */
struct frame_table_entry *lookup_page_frame (void *kpage);

/* Function for frame eviction */
bool evict_page_frame (void);

/* Move the element in lru_cache to front */
void lru_move_to_front (void *);

#endif /* vm/frame.h */
