#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <user/syscall.h>
#include <string.h>
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "process.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "pagedir.h"
#include "threads/synch.h"
#include "devices/input.h"

static void syscall_handler (struct intr_frame *);

void check_esp(void const *esp);
void release_all_locks(struct thread *t);

struct fileWithFd{
    int fd;
    struct file *f;
};

struct fileWithFd fileFdArray[128];
int currentFd = 1;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

// for user access memory
void check_esp(void const *esp) {

    if (!(is_user_vaddr(esp) &&
    pagedir_get_page(thread_current()->pagedir, esp) &&
    esp != NULL)) {
       exit(-1);
    }

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
  check_esp(f->esp);
//    release_all_locks(t);
 //   pagedir_clear_page(f->esp, t->pagedir);

  // ---------------------------------

  int syscall_num;
  syscall_num = *(int *)f->esp;
//  f->esp += 1;

  void *fst = (int *)(f->esp) + 1;
  void *snd = (int *)(f->esp) + 2;
  void *trd = (int *)(f->esp) + 3;

//  printf("fst = %d| snd = %i| trd = %i|\n", *(int *)fst, *(int *)snd, *(int *)trd);

  switch (syscall_num) {
    case SYS_EXEC:
      check_esp(fst);
      f->eax = (uint32_t) exec((char *)fst);
      break;
    case SYS_CLOSE:
      check_esp(fst);
      close((int)fst);
      break;
    case SYS_CREATE:
      check_esp(fst);
      check_esp(snd);
      f->eax = (uint32_t) create(fst, (unsigned int) snd);
      break;
    case SYS_EXIT:
      exit(t->status);
      break;
    case SYS_FILESIZE:
      check_esp(fst);
      f->eax = (uint32_t) filesize((int) fst);
      break;
    case SYS_HALT:
      halt();
    case SYS_OPEN:
      check_esp(fst);
      f->eax = (uint32_t) open(fst);
      break;
    case SYS_READ:
      check_esp(fst);
      check_esp(snd);
      check_esp(trd);
      f->eax = (uint32_t) read(*(int *)fst, (void *)*(int *)snd, *(unsigned *) trd);
      break;
    case SYS_WRITE:
      check_esp(fst);
      check_esp(snd);
      check_esp(trd);
      f->eax = (uint32_t) write(*(int *)fst, *(void **)snd, *(unsigned *) trd);
      break;
    case SYS_WAIT:
      check_esp(fst);
      f->eax = (uint32_t) wait((pid_t) fst);
      break;
    case SYS_REMOVE:
      check_esp(fst);
      f->eax = (uint32_t) remove(fst);
      break;
    case SYS_SEEK:
      check_esp(fst);
      check_esp(snd);
      seek((int) fst, (unsigned int) snd);
      break;
    case SYS_TELL:
      check_esp(fst);
      f->eax = tell((int) fst);
      break;
    default:break;
  }
}

void
halt(void) {
  shutdown_power_off();
}

void
exit (int status) {
  struct thread *cur = thread_current();
  printf("%s: exit(%d)\n", thread_current()->name, thread_current()->status);
//  thread_unblock(cur->parent);
//  process_exit();
  thread_exit();
}

pid_t
exec(const char *cmd_line) {

}

int
wait(pid_t pid) {
  return process_wait(pid);
}

bool
create(const char *file, unsigned initial_size) {

}

bool
remove(const char *file) {

}

int
open(const char *file) {
  struct file *file1 = filesys_open(file);
  if (currentFd<129) {
    currentFd++;
    struct fileWithFd *fwfd = (currentFd, file1);
    fileFdArray[currentFd-2] = *fwfd;
    // will use a size 130 array of file better than create a sturct for it?
  }
  return currentFd;
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
  if (fd == 1) {

  }

  if (fd == 0) {
    input_getc();
  }
}

int
write(int fd, const void *buffer, unsigned size) {

//  printf("** inwrite(), fd = %d, buffer = 0x%d, size = %u\n", fd, buffer, size);

  if (fd == 1) {
    putbuf(buffer, size);
    return 0;
  }

//  int current = file_tell(fd);
//  if (current < file_length(fd)) {
//    return file_write_at(fd, buffer, size, current);
//  } else {
//    return 0;
//  }
  return 0;
}

void
seek(int fd, unsigned position) {
  file_seek(fileFdArray[fd-2].f, position);
}

unsigned
tell(int fd) {
  return (unsigned int) file_tell(fileFdArray[fd - 2].f);
}

void
close(int fd) {
  if (fd<129) {
    file_close(fileFdArray[fd-2].f);
  }
}