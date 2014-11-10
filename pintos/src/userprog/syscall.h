#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#define USER_VADDR_START ((void *) 0x08048000)
#define STACK_LIMIT 8*1024*1024
#define MAX_STACK_ADDR (PHYS_BASE - STACK_LIMIT)

struct lock filesys_lock;

void syscall_init (void);

#endif /* userprog/syscall.h */
