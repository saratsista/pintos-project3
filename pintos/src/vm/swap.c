#include "devices/block.h"
#include "vm/swap.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "threads/synch.h"

void
init_swap_table ()
{
  struct block *swap_space = block_get_role (BLOCK_SWAP);
  
  if (!swap_space)
    return;

  size_t bit_cnt = (block_size (swap_space) * BLOCK_SECTOR_SIZE)/PGSIZE;   
  swap_table = bitmap_create (bit_cnt);

  if (!swap_table)
     return;
}

/* Returns the block sector number for the allocated swap slot.
   Returns -1 if no sector can be allocated */
size_t
write_to_swap (void *kvaddr)
{
  lock_acquire (&swap_lock);
  /* set the bit in the bitmap_table */
  size_t frame_bit = bitmap_scan_and_flip (swap_table, 0, 1, false);
  if (frame_bit == BITMAP_ERROR)
   {
     lock_release (&swap_lock);
     return -1;
   }
  lock_release (&swap_lock);

  /* write to the sector */
  block_sector_t sector = frame_bit * SECTORS_PER_FRAME;
  block_write (block_get_role (BLOCK_SWAP), sector, kvaddr);
    
  return frame_bit;
}

/* free_swap_slot */
void
read_from_swap (size_t swap_index, void *kvaddr)
{
/*
 struct frame_table_entry *fte = lookup_page_frame (kvaddr);
 if (fte == NULL)
   return;
 size_t frame_bit = fte->spte->swap_index; 
*/
 block_sector_t sector = swap_index * SECTORS_PER_FRAME;
 block_read (block_get_role (BLOCK_SWAP), sector, kvaddr);
  
 /* Reset bit in the swap_table */
 lock_acquire (&swap_lock);
 bitmap_reset (swap_table, swap_index);
 lock_release (&swap_lock);
} 

