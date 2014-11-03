#include <hash.h>
#include <list.h>
#include "frame.h"

/* Possible locations where required data might be 
   present. */
enum data_location
  {
    ON_FRAME,
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
  struct frame_table_entry *fte;// A frame_table_entry for this page
  struct file *file;		// The executable file
  uint32_t file_page;		// which page of the executable? 
  struct list alias_frames;     // Other frames that map to same page frame
  struct hash_elem hash_elem;  
  /*
  1. Info for eviction policy
  2. Mapping from swap pages to disk
  3. locations of memory mapped files
  4. Where to find the exec and which page to load?
  5. How to get the swap disk and which sector of swap disk? 
  6. The list of alias_frames can be avoided. See pintos docs.
 */
}

hash_hash_func sup_page_hash;
hash_less_func sup_page_less;
void *add_to_spt (void *vaddr);
void free_spt_entry (void *vaddr);
