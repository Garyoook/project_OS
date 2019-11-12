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
#include "threads/malloc.h"

#define FILE_LIMIT 128
#define STD_IO 2

static void syscall_handler (struct intr_frame *);

bool safe_access(void const *esp);
void release_all_locks(struct thread *t);

struct fileWithFd{
    int fd;
    struct file *f;
};

struct fileWithFd fileFdArray[FILE_LIMIT];
tid_t tidArray[FILE_LIMIT];
//bool canBeWritten[FILE_LIMIT];
bool reopen[FILE_LIMIT];
int currentFd = STD_IO;

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

// for user access memory
bool safe_access(void const *esp) {
  return (is_user_vaddr(esp) &&
    pagedir_get_page(thread_current()->pagedir, esp) &&
    esp != NULL);

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
  // check esp is valid:
  if (!safe_access(f->esp)) exit(EXIT_FAIL);

  // for user access memory:
  struct thread *t = thread_current();
//    release_all_locks(t);
//  pagedir_clear_page(f->esp, t->pagedir);

  // ---------------------------------

  int syscall_num;
  syscall_num = *(int *)f->esp;

  // get all 3 possible arguments
  // would'nt affect anything if it is not argument.
  void **fst = (void **)(f->esp) + 1;
  void **snd = (void **)(f->esp) + 2;
  void **trd = (void **)(f->esp) + 3;

  switch (syscall_num) {
    case SYS_EXEC:
      if (!safe_access(fst)) exit(EXIT_FAIL);
      f->eax = (uint32_t) exec(*(char **)fst);
      break;
    case SYS_CLOSE:
      if (!safe_access(fst)) exit(EXIT_FAIL);
      close(*(int *)fst);
      break;
    case SYS_CREATE:
      if (!safe_access(fst)) exit(EXIT_FAIL);
      if (!safe_access(snd)) exit(EXIT_FAIL);
      f->eax = (uint32_t) create(*(char **)fst, (void *)*(int *)snd);
      break;
    case SYS_EXIT:

      if (!safe_access(fst)) exit(EXIT_FAIL);//must check it here to pass sc-bad-arg
      exit(*(int*)fst);
      break;
    case SYS_FILESIZE:
      if (!safe_access(fst)) exit(EXIT_FAIL);
      f->eax = (uint32_t) filesize(*(int *) fst);
      break;
    case SYS_HALT:
      halt();
    case SYS_OPEN:
      if (!safe_access(fst)) exit(EXIT_FAIL);
      if (!safe_access(*(char **)fst)) exit(EXIT_FAIL);
      f->eax = (uint32_t) open(*(char **)fst);
      break;
    case SYS_READ:
      if (!safe_access(fst)) exit(EXIT_FAIL);
      if (!safe_access(snd)) exit(EXIT_FAIL);
      if (!safe_access(trd)) exit(EXIT_FAIL);
      f->eax = (uint32_t) read(*(int *)fst, *snd, *(unsigned *) trd);
      break;
    case SYS_WRITE:
      if (!safe_access(fst)) exit(EXIT_FAIL);
      if (!safe_access(snd)) exit(EXIT_FAIL);
      if (!safe_access(trd)) exit(EXIT_FAIL);
      f->eax = (uint32_t) write(*(int *)fst, *snd, *(unsigned *) trd);
      break;
    case SYS_WAIT:
      if (!safe_access(fst)) exit(EXIT_FAIL);
      f->eax = (uint32_t) wait(*(pid_t *)fst);
      break;
    case SYS_REMOVE:
      if (!safe_access(fst)) exit(EXIT_FAIL);
      if (!safe_access(*(char **)fst)) exit(EXIT_FAIL);

      f->eax = (uint32_t) remove(*(const char **) fst);
      break;
    case SYS_SEEK:
      if (!safe_access(fst)) exit(EXIT_FAIL);
      if (!safe_access(snd)) exit(EXIT_FAIL);
      seek(*(int *) fst, *(unsigned int *) snd);
      break;
    case SYS_TELL:
      if (!safe_access(fst)) exit(EXIT_FAIL);
      f->eax = tell(*(int *) fst);
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
  enum intr_level old_level;
  old_level = intr_disable();

  struct thread *cur = thread_current();
  printf("%s: exit(%d)\n", cur->name, status);
  struct thread *parent = thread_current()->parent;
  if (parent != NULL){
    parent->child_process_tid[parent->child_pos] = thread_current()->tid;
    parent->child_process_exit_status[parent->child_pos] = status;
    parent->child_pos++;
    parent->count--;
    thread_unblock(cur->parent);
  }

  intr_set_level(old_level);

  thread_exit();
}

pid_t
exec(const char *cmd_line) {
  if (!safe_access(cmd_line)) {
    return EXIT_FAIL;
  }

  pid_t pid = process_execute(cmd_line);
  return pid;
}

int
wait(pid_t pid) {
  enum intr_level old_level;
  old_level = intr_disable();

  for (int i = 0; i< CHILD_P_NUM; i++){
    if (thread_current()->child_process_tid[i] == pid){
      return EXIT_FAIL;
    }
  }
  struct thread *t = lookup_tid(pid);
  int result = process_wait(pid);

  intr_set_level(old_level);

  return result;

}

bool
create(const char *file, unsigned initial_size) {
  if (strnlen(file, 15) > 14) {
    return false;
  }
  if (initial_size >= 0 && *file != '\0') {
    return filesys_create(file, initial_size);
  } else {
    exit(EXIT_FAIL);
  }
}

bool
remove(const char *file) {
  return filesys_remove(file);
}

int
open(const char *file) {
  bool reopened = false;
  struct file *file1 = filesys_open(file);
  if (!strcmp(file, current_file_name)) {
    reopened = true;
  }

  if (strlen(file) == 0 || file1 == NULL) {
    return EXIT_FAIL;
  }

  if (currentFd <= FILE_LIMIT) {
    currentFd++;
    fileFdArray[currentFd-STD_IO].fd = currentFd;
    fileFdArray[currentFd-STD_IO].f = file1;
    tidArray[currentFd-STD_IO] = thread_current()->tid;
    reopen[currentFd-STD_IO] = reopened;
//    canBeWritten[currentFd-STD_IO] = true;
  } else {
    return EXIT_FAIL;
  }

  file_deny_write(file1);
  return currentFd;
}

int
filesize(int fd) {
  if (fileFdArray[fd-STD_IO].f == NULL) {
    exit(EXIT_FAIL);
  }
  return file_length(fileFdArray[fd-STD_IO].f);
}

int
read(int fd, void *buffer, unsigned size) {
  if (fd < 1 || fd > FILE_LIMIT) {
    exit(EXIT_FAIL);
  }
  if (tidArray[fd-STD_IO] != thread_current()->tid) {
    exit(EXIT_FAIL);
  }
  if (!safe_access(buffer)) exit(EXIT_FAIL);
  if (fd == 0) {
    return input_getc();
  }
//  canBeWritten[fd-STD_IO] = false;

  struct file *currentFile = fileFdArray[fd-STD_IO].f;

  if (currentFile != NULL) {
//    canBeWritten[fd-STD_IO] = true;
    int id = file_read(currentFile, buffer, size);
    return id;
  } else {
    exit(EXIT_FAIL);
  }
}

int
write(int fd, const void *buffer, unsigned size) {
  if (fd < 1 || fd > FILE_LIMIT) {
    exit(EXIT_FAIL);
  }
  if (fd == 1) {
    // size may not bigger than hundred bytes
    // otherwise may confused
    putbuf(buffer, size);
    return 0;
  }

  if (tidArray[fd-STD_IO] != thread_current()->tid || reopen[fd-STD_IO]) {
//    exit(EXIT_FAIL);
  } else {
    file_allow_write(fileFdArray[fd-STD_IO].f);
  }

  int result = file_write(fileFdArray[fd-STD_IO].f, buffer, size);

  file_deny_write(fileFdArray[fd-STD_IO].f);
  return result;
}

void
seek(int fd, unsigned position) {
  file_seek(fileFdArray[fd-STD_IO].f, position);
}

unsigned
tell(int fd) {
  return (unsigned int) file_tell(fileFdArray[fd-STD_IO].f);
}

void
close(int fd) {
  if (fileFdArray[fd-STD_IO].f == NULL){
    return;
  }
  if (tidArray[fd-STD_IO] != thread_current()->tid) {
    return;
  }
  if (fd <= FILE_LIMIT) {
//    canBeWritten[fd-STD_IO] = true;
    file_close(fileFdArray[fd-STD_IO].f);
    fileFdArray[fd-STD_IO].f = NULL;
  } else {
    return;
  }
}