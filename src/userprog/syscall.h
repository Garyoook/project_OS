#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#define EXIT_FAIL -1

#include <user/syscall.h>
#include "threads/thread.h"

struct child {
  tid_t tid;
  int exit_status;
  struct semaphore child_sema;
  struct list_elem child_elem;
};


struct lock syscall_lock;
char *current_file_name;
int num;
void syscall_init (void);
void exit (int status);
bool safe_access(void const *esp);

#endif /* userprog/syscall.h */
