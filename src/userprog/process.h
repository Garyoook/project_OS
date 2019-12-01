#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H


#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "syscall.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "threads/thread.h"

#define STACK_LIMIT 4096
#define ARGC 16
#define FILE_NAME_LEN_LIMIT 15

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
static bool install_page (void *upage, void *kpage, bool writable);
bool load_segment (struct file *file, off_t ofs, uint8_t *upage,
                   uint32_t read_bytes, uint32_t zero_bytes,
                   bool writable);
bool setup_stack(void **esp);
#endif /* userprog/process.h */
