#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

#define STACK_LIMIT 4096
#define ARGC 16
#define FILE_NAME_LEN_LIMIT 15

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
#endif /* userprog/process.h */
