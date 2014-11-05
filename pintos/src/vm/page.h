#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include <list.h>
#include "frame.h"

/* Possible locations where required data might be 
   present. */
enum data_location
  {
    ON_FILE,
    ON_SWAP,
    ON_DISK
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
  off_t file_off;		// The offset in the executable
  bool writable;		// Is the page writable?
  size_t read_bytes;
  size_t zero_bytes;
  struct list alias_frames;     // Other frames that map to same page frame
  struct hash_elem elem;  
  /*
  1. Info for eviction policy
  2. Mapping from swap pages to disk
  3. locations of memory mapped files
  4. Where to find the exec and which page to load?
  5. How to get the swap disk and which sector of swap disk? 
  6. The list of alias_frames can be avoided. See pintos docs.
  7. Pointer to frame_table_entry?
 */
};

hash_hash_func sup_page_hash;
hash_less_func sup_page_less;
bool add_to_spt (struct file *, off_t, uint8_t *, bool, size_t, size_t);
void free_spt_entry (void *);
struct sup_page_entry *lookup_sup_page (void *vaddr);

#endif /* vm/page.h */
