#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#define USER_VADDR_START ((void *)0x08084000)

struct lock filesys_lock;

void syscall_init (void);

#endif /* userprog/syscall.h */
