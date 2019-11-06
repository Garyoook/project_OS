#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <user/syscall.h>
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "process.h"
#include "filesys/file.h"
#include "pagedir.h"
#include "threads/synch.h"

static void syscall_handler (struct intr_frame *);
bool check_esp(void const *esp);
void release_all_locks(struct thread *t);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

// for user access memory
bool check_esp(void const *esp) {
  for (int i = 0; i < 4; i++){
    if (!(is_user_vaddr(esp + i) &&
    pagedir_get_page(thread_current()->pagedir, esp + i) &&
    esp+i != NULL)) {
      return false;
    }
  }
  return true;
}

void release_all_locks(struct thread *t){
  struct list_elem *e,*next;
  e = list_begin (&t->locks);

  while ((next = list_next (e)) != list_end (&t->locks)){
    struct lock *thisLock = list_entry (e, struct lock, lockElem);
    lock_release(thisLock);

    e = next;
  }
}
// ---------------------------------

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  // for user access memory
  if (!check_esp(f->esp)){
    release_all_locks(thread_current());
    pagedir_clear_page(f->esp, thread_current()->pagedir);

    exit(-1);
  }
  // ---------------------------------
  printf ("system call!\n");
  thread_exit ();
}

void
halt(void) {
  shutdown_power_off();
}

void
exit (int status) {
  printf("%s: exit(%d)\n", thread_current()->name, thread_current()->status);
  process_exit();
}

pid_t
exec(const char *cmd_line) {

}

int
wait(pid_t pid) {
  struct thread *t = &pid;
  if (t->status == THREAD_DYING){
    return THREAD_DYING;
  }
}

bool
create(const char *file, unsigned initial_size) {

}

bool
remove(const char *file) {

}

int
open(const char *file) {

}

int
filesize(int fd) {
  int bits=0;
  while (fd>0) {
    bits++;
    fd /= 2;
  }
  return bits/8;
}

int
read(int fd, void *buffer, unsigned size) {

}

int
write(int fd, const void *buffer, unsigned size) {

  if (fd == 1) {
    putbuf(buffer, size);
  }

  int current = file_tell(fd);
  if (current < file_length(fd)) {
    return file_write_at(fd, buffer, size, current);
  } else {
    return 0;
  }

}

void
seek(int fd, unsigned position) {

}

unsigned
tell(int fd) {

}

void
close(int fd) {
  
}