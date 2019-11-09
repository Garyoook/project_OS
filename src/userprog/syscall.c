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
int currentFd = 2;

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

  void **fst = (void **)(f->esp) + 1;
  void **snd = (void **)(f->esp) + 2;
  void **trd = (void **)(f->esp) + 3;

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
      check_esp(*(const char **)fst);
      check_esp(snd);
      f->eax = (uint32_t) create(*(char **)fst, (void *)*(int *)snd);
      break;
    case SYS_EXIT:
      exit(*(int*)fst);
      break;
    case SYS_FILESIZE:
      check_esp(fst);
      f->eax = (uint32_t) filesize((int) fst);
      break;
    case SYS_HALT:
      halt();
    case SYS_OPEN:
      check_esp(fst);
      check_esp(*fst);
      f->eax = (uint32_t) open(*(char **)fst);
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
  printf("%s: exit(%d)\n", cur->name, status);
  thread_exit();
}

pid_t
exec(const char *cmd_line) {
  struct thread *child_process;
  return 0;
}

int
wait(pid_t pid) {
  return process_wait(pid);
}

bool
create(const char *file, unsigned initial_size) {
  if (strnlen(file, 15) > 14) {
    return false;
  }
  if (initial_size >= 0 && *file != '\0') {
    return filesys_create(file, initial_size);
  } else {
    exit(-1);
  }
}

bool
remove(const char *file) {
  return filesys_remove(file);
}

int
open(const char *file) {
  struct file *file1 = filesys_open(file);

  if (strlen(file) == 0 || file1 == NULL) {
    return -1;
  }

  if (currentFd<129) {
    currentFd++;
    fileFdArray[currentFd-2].fd = currentFd;
    fileFdArray[currentFd-2].f = file1;
    // will use a size 130 array of file better than create a sturct for it?
  } else {
    return -1;
  }
  return currentFd;
}

int
filesize(int fd) {
  return file_length(fileFdArray[fd-2].f);
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

  if (fd<1 || fd>130) {
    exit(-1);
  }

  check_esp(buffer);

  if (fd == 1) {
    // size may not bigger than hundred bytes
    // otherwise may confused
    putbuf(buffer, size);
    return 0;
  }

  struct file *currentFile = fileFdArray[fd-2].f;
  int current = file_tell(currentFile);
  return file_write_at(currentFile, buffer, size, current);
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