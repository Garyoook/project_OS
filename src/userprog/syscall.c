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
  struct thread *t = thread_current();
  if (!check_esp(f->esp)){
    release_all_locks(t);
    pagedir_clear_page(f->esp, t->pagedir);
    exit(-1);
  }
  // ---------------------------------

  int syscall_num;
  syscall_num = *(int *)f->esp;
  f->esp += 1;

  void *fst = f->esp++;
  void *snd = f->esp++;
  void *trd = f->esp++;
  void *fot = f->esp++;


  switch (syscall_num) {
    case SYS_EXEC:
      f->eax = (uint32_t) exec((char *)fst);
      break;
    case SYS_CLOSE:
      close((int)fst);
      break;
    case SYS_CREATE:
      f->eax = (uint32_t) create(fst, (unsigned int) snd);
      break;
    case SYS_EXIT:
      exit(t->status);
      break;
    case SYS_FILESIZE:
      f->eax = (uint32_t) filesize((int) fst);
      break;
    case SYS_HALT:
      halt();
    case SYS_OPEN:
      f->eax = (uint32_t) open(fst);
      break;
    case SYS_READ:
      f->eax = (uint32_t) read((int) fst, snd, (unsigned int) trd);
      break;
    case SYS_WRITE:
      f->eax = (uint32_t) write((int) fst, snd, (unsigned int) trd);
      break;
    case SYS_WAIT:
      f->eax = (uint32_t) wait((pid_t) fst);
      break;
    case SYS_REMOVE:
      f->eax = (uint32_t) remove(fst);
      break;
    case SYS_SEEK:
      seek((int) fst, (unsigned int) snd);
      break;
    case SYS_TELL:
      f->eax = tell((int) fst);
      break;
    default:break;
  }
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