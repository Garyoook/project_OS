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

bool safe_access(void const *esp);
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
bool safe_access(void const *esp) {
  return (is_user_vaddr(esp) &&
    pagedir_get_page(thread_current()->pagedir, esp) &&
    esp != NULL);

}

bool ptr_invalid(const void *ptr) {
  return (!is_user_vaddr(ptr) || ptr == NULL);
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
  if (!safe_access(f->esp)) exit(-1);

  // for user access memory:
  struct thread *t = thread_current();
//    release_all_locks(t);
//  pagedir_clear_page(f->esp, t->pagedir);

  // ---------------------------------

  int syscall_num;
  syscall_num = *(int *)f->esp;

  void **fst = (void **)(f->esp) + 1;
  void **snd = (void **)(f->esp) + 2;
  void **trd = (void **)(f->esp) + 3;

  switch (syscall_num) {

    case SYS_EXEC:
//      check_esp(fst);
//      check_esp(*(char **)fst);
      f->eax = (uint32_t) exec(*(char **)fst);
      break;
    case SYS_CLOSE:
//      check_esp(fst);
      close((int)fst);
      break;
    case SYS_CREATE:
//      check_esp(fst);
//      check_esp(*(char **)fst);
//      check_esp(snd);
      f->eax = (uint32_t) create(*(char **)fst, (void *)*(int *)snd);
      break;
    case SYS_EXIT:

      if (!safe_access(fst)) exit(-1);//must check it here to pass sc-bad-arg
      exit(*(int*)fst);
      break;
    case SYS_FILESIZE:
//      check_esp(fst);
      f->eax = (uint32_t) filesize(*(int *) fst);
      break;
    case SYS_HALT:
      halt();
    case SYS_OPEN:
      if (!safe_access(fst)) exit(-1);
      if (!safe_access(*(char **)fst)) exit(-1);
      f->eax = (uint32_t) open(*(char **)fst);
      break;
    case SYS_READ:
//      check_esp(fst);
//      check_esp(snd);
//      check_esp(*snd);
//      check_esp(trd);
      f->eax = (uint32_t) read(*(int *)fst, *snd, *(unsigned *) trd);
      break;
    case SYS_WRITE:
//      check_esp(fst);
//      check_esp(snd);
//      check_esp(*snd);
//      check_esp(trd);
      f->eax = (uint32_t) write(*(int *)fst, *snd, *(unsigned *) trd);
      break;
    case SYS_WAIT:
//      check_esp(fst);
      f->eax = (uint32_t) wait(*(pid_t *)fst);
      break;
    case SYS_REMOVE:
//      check_esp(fst);
      if (!safe_access(*(char **)fst)) exit(-1);

      f->eax = (uint32_t) remove(*(const char **) fst);
      break;
    case SYS_SEEK:
//      check_esp(fst);
//      check_esp(snd);
      seek(*(int *) fst, *(unsigned int *) snd);
      break;
    case SYS_TELL:
//      check_esp(fst);
      f->eax = tell(*(int *) fst);
      break;
    default:break;
  }
}

void
halt(void) {
  // printf(NULL)
  shutdown_power_off();
}

void
exit (int status) {
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
//  if (parent != NULL){
//  for (int i = 0; i < 100; i++){
//    if (parent->child_process_tid[i] == thread_current()->tid){
//      printf("cadsfsadfdsafdsaf  %d", parent->child_process_exit_status[i]);
//    }}
//  }

  thread_exit();
}

pid_t
exec(const char *cmd_line) {
  if (!safe_access(cmd_line)) {
    return -1;
  }

  pid_t pid = process_execute(cmd_line);

//  if (pid == TID_ERROR) {
//    pid = -1;
//  }
//
//  if(wait(tid) != -1) {
//    return tid;
//  }
  return pid;
}

int
wait(pid_t pid) {
  for (int i = 0; i< 100; i++){
    if (thread_current()->child_process_tid[i] == pid){
      return -1;
    }
  }


  struct thread *t = lookup_tid(pid);
  int result = process_wait(pid);

  return result;

}

bool
create(const char *file, unsigned initial_size) {
  // printf(NULL)
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
  // printf(NULL)
  return filesys_remove(file);
}

int
open(const char *file) {
  // printf(NULL)
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
  // printf(NULL)
  return file_length(fileFdArray[fd-2].f);
}

int
read(int fd, void *buffer, unsigned size) {
  // printf(NULL)
  if (fd<1 || fd>130) {
    exit(-1);
  }
  if (!safe_access(buffer)) exit(-1);
  if (fd == 0) {
    return input_getc();
  }

  struct file *currentFile = fileFdArray[fd-2].f;
  if (currentFile != NULL) {
    return file_read(currentFile, buffer, size);
  } else {
    exit(-1);
  }
}

int
write(int fd, const void *buffer, unsigned size) {
  if (fd<1 || fd>130) {
    exit(-1);
  }
  if (!safe_access(buffer)) exit(-1);
  if (fd == 1) {
    // size may not bigger than hundred bytes
    // otherwise may confused
    putbuf(buffer, size);
    return 0;
  }

  printf("fd:%d", fd);
  return file_write(fileFdArray[fd-2].f, buffer, size);

}

void
seek(int fd, unsigned position) {
  // printf(NULL)
  file_seek(fileFdArray[fd-2].f, position);
}

unsigned
tell(int fd) {
  // printf(NULL)
  return (unsigned int) file_tell(fileFdArray[fd - 2].f);
}

void
close(int fd) {
//  // printf(NULL)

  printf("close fd:%d", fd);
//  if (fd<129) {
//    file_close(fileFdArray[fd-2].f);
//  }
}