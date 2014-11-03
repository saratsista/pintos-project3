#include <hash.h>
#include <stdint.h>

/* Frame Table: Frame table is a global data structure and 
   is common for all processes.*/
struct hash frame_table;

/* Structure for each entry in the Frame Table */
struct frame_table_entry
{
  void *vaddr;		   /* User virtual address */
  void *kvaddr;		   /* Kernel virtual address for vaddr */
  struct hash_elem elem;
};

/* Initialize the frame_table */
bool init_frame_table ();

/* A function that calculates hash for each frame_table_entry */
hash_hash_func frame_table_hash;

/* Returns true if frame_table_entry A precedes frame_table_entry B */
hash_less_func frame_less_func;

/* Allocate a page frame for VADDR and install the
   mapping in the frame_table */
void *allocate_page_frame (void *vaddr);

/* Free a page frame VADDR from the frame table */
void free_page_frame (void *vaddr);
