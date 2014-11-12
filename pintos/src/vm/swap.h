#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <bitmap.h>
#include "threads/vaddr.h"

#define SECTORS_PER_FRAME (PGSIZE/BLOCK_SECTOR_SIZE)

/* Swap Table. Golbal data structure*/
struct bitmap *swap_table;

struct lock swap_lock; 

void init_swap_table (void);
size_t write_to_swap (void *kvaddr);
void read_from_swap (size_t swap_index, void *kvaddr);

#endif /* VM_SWAP_H */
