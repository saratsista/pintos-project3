#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include <list.h>
#include <user/syscall.h>
#include "filesys/inode.h"
#include "frame.h"

/* Possible locations where required data might be 
   present. */
enum data_location
  {
    ON_FILE,
    ON_SWAP,
    MMAP
  };

/* Page table entry for Supplementary Page Table.
   The supplementary page table must be a per-process 
   data structure. */
struct sup_page_entry
{
  enum data_location location;	// Location where data is present
  void *vaddr;			// User virtual address for the faulted page
  void *kvaddr;			// kernel virtual address for VADDR
  struct file *file;		// The executable file
  struct inode *inode;
  uint32_t file_off;		// The offset in the executable
  bool writable;		// Is the page writable?
  size_t read_bytes;
  size_t zero_bytes;
  bool is_loaded;
  struct hash_elem elem;  
  /*
  1. Info for eviction policy
  2. Mapping from swap pages to disk
  3. locations of memory mapped files
  5. How to get the swap disk and which sector of swap disk? 
  6. The list of alias_frames can be avoided. See pintos docs.
 */
};

/* A list for mmapped pages. This is a per-process DS */
struct map_page
{
  struct sup_page_entry *spte;	// The spt entry for the mapped page
  mapid_t mapid;		// the mapid for this page. 
  struct list_elem map_elem;
};

/* A lock for allocating mapid */
struct lock mapid_lock;

hash_hash_func sup_page_hash;
hash_less_func sup_page_less;
/* function which deletes each element in sup_page_table */
hash_action_func sup_page_destroy;

struct sup_page_entry *add_to_spt (struct file *, uint32_t, uint8_t *, bool, size_t, size_t, enum data_location);
struct sup_page_entry *lookup_sup_page (void *vaddr);
void free_spt_entry (struct sup_page_entry *);
void spt_destroy (struct hash *);
void update_map_table (struct sup_page_entry *);

#endif /* vm/page.h */
