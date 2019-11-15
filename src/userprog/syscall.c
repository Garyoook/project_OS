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
#define FD_INIT 2

static void syscall_handler (struct intr_frame *);

bool safe_access(void const *esp);
// note that we did not use release_all_locks() because we have
// dealt with all the locks in exit(), keep using it will cause kernel panic;
void release_all_locks(struct thread *t);

// a struct to keep track of a file with its name, fd, tid and open status.
struct fileWithFd {
  struct file *f;
  const char *name;
  int fd;
  tid_t tid;
  bool reopened;
  struct list_elem file_elem;
};

// the initial file descriptor number;
int currentFd = FD_INIT;

void
syscall_init (void)
{
  lock_init(&wait_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

// for user access memory
bool safe_access(void const *esp) {
  return (is_user_vaddr(esp) &&
    pagedir_get_page(thread_current()->pagedir, esp) &&
    esp != NULL);
}

void release_all_locks(struct thread *t){
  struct list_elem *e;
  e = list_begin (&t->locks);

  while (e != list_end (&t->locks)) {
    struct lock *thisLock = list_entry (e, struct lock, lockElem);
    lock_release(thisLock);
    e = e->next;
  }
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
  // check esp is valid:
  if (!safe_access(f->esp)) exit(EXIT_FAIL);

  struct thread *t = thread_current();
  // mark the thread as "in syscall" for exception checking;
  t->in_syscall = true;

  // syscall is at the top of stack now:
  int syscall_num;
  syscall_num = *(int *)f->esp;

  // get all 3 possible arguments
  // would'nt affect anything if not all of them are arguments in this syscall
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
      if (!safe_access(fst)) exit(EXIT_FAIL);
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
      if (!safe_access(*snd)) exit(EXIT_FAIL);
      if (!safe_access(trd)) exit(EXIT_FAIL);
      f->eax = (uint32_t) read(*(int *)fst, *snd, *(unsigned *) trd);
      break;
    case SYS_WRITE:
      if (!safe_access(fst)) exit(EXIT_FAIL);
      if (!safe_access(snd)) exit(EXIT_FAIL);
      if (!safe_access(*snd)) exit(EXIT_FAIL);
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
  return process_wait(pid);
}

// halt: power off the system.
void
halt(void) {
  shutdown_power_off();
}

void
exit (int status) {
  enum intr_level old_level;
  old_level = intr_disable();
  struct thread *cur = thread_current();
  // termination message:
  printf("%s: exit(%d)\n", cur->name, status);

  struct thread *parent = cur->parent;
  if (parent != NULL) {
    struct list_elem *e = list_begin(&parent->child_list);
    while (e != list_end(&parent->child_list)){
      struct child *thr_child = list_entry(e, struct child, child_elem);
      if (cur->tid == thr_child->tid) {
        thr_child->exit_status = status;
        sema_up(&thr_child->child_sema);
        break;
      }
      e = e->next;
    }
  }

  // free the child list;
  struct list_elem *e = list_begin(&cur->child_list);
  while (e != list_end(&cur->child_list)){
    struct child *thr_child = list_entry(e, struct child, child_elem);
    list_remove(e);
    e = e->next;
    free(thr_child);
  }

  // free the file list;
  struct list_elem *ef = list_begin(&cur->file_fd_list);
  while (ef != NULL && ef != list_end(&cur->file_fd_list)){
    struct fileWithFd *fileFd = list_entry(ef, struct fileWithFd, file_elem);
    list_remove(ef);
    ef = ef->next;
    free(fileFd);
  }

  // if exit by an error, release all locks.
  if (status == EXIT_FAIL) {
    release_all_locks(thread_current());
  }

  intr_set_level(old_level);
  thread_exit();
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
  struct thread *cur = thread_current();

  if (!strcmp(file, cur->name)) {
    reopened = true;
  }
  if (!strcmp(file, current_file_name)) {
    reopened = true;
  }

  if (strlen(file) == 0 || file1 == NULL) {
    return EXIT_FAIL;
  }

  if (currentFd <= FILE_LIMIT) {
    lock_acquire(&wait_lock);
    currentFd++;
    lock_release(&wait_lock);

    // initialize the fields of a created file and push it into
    // the file list of current thread.
    struct fileWithFd *fileFd =
        (struct fileWithFd *) malloc(sizeof(struct fileWithFd));
    if (fileFd == NULL) {
      return EXIT_FAIL;
    }
    fileFd->name = file;
    fileFd->fd = currentFd;
    fileFd->f = file1;
    fileFd->tid = cur->tid;
    fileFd->reopened = reopened;
    if (fileFd->reopened) {
      file_reopen(fileFd->f);
    }

    list_push_back(&cur->file_fd_list, &fileFd->file_elem);
  } else {
    return EXIT_FAIL;
  }

  file_deny_write(file1);
  return currentFd;
}

int
filesize(int fd) {
  struct thread *cur = thread_current();
  struct list_elem *e = list_begin(&cur->file_fd_list);
  while (e != list_end(&cur->file_fd_list)){
    struct fileWithFd *fileFd = list_entry(e, struct fileWithFd, file_elem);
    if (fileFd->fd == fd) {
      if (fileFd->f == NULL) {
        exit(EXIT_FAIL);
      }
      return file_length(fileFd->f);
    }
    e = e->next;
  }
}

int
read(int fd, void *buffer, unsigned size) {
  if (fd < STDOUT_FILENO || fd > FILE_LIMIT) {
    exit(EXIT_FAIL);
  }
  if (fd == STDIN_FILENO) {
    return input_getc();
  }

  struct thread *cur = thread_current();
  struct list_elem *e = list_begin(&cur->file_fd_list);
  while (e != list_end(&cur->file_fd_list)){
    struct fileWithFd *fileFd = list_entry(e, struct fileWithFd, file_elem);
    if (fileFd->fd == fd) {
        if (fileFd->tid != cur->tid) {
          exit(EXIT_FAIL);
        }
      if (fileFd->f != NULL) {
        return file_read(fileFd->f, buffer, size);
      } else {
        exit(EXIT_FAIL);
      }
    }
    e = e->next;
  }
}

int
write(int fd, const void *buffer, unsigned size) {
  if (fd < STDOUT_FILENO || fd > FILE_LIMIT) {
    exit(EXIT_FAIL);
  }
  if (fd == STDOUT_FILENO) {
    putbuf(buffer, size);
    return 0;
  }

  struct thread *cur = thread_current();
  struct list_elem *e = list_begin(&cur->file_fd_list);
  while (e != list_end(&cur->file_fd_list)){
    struct fileWithFd *fileFd = list_entry(e, struct fileWithFd, file_elem);
    if (fileFd->fd == fd ) {
      if (fileFd->tid != cur->tid) {
        exit(EXIT_FAIL);
      } else {
        if (!fileFd->reopened)
          file_allow_write(fileFd->f);
      }
      int result = file_write(fileFd->f, buffer, size);
      file_deny_write(fileFd->f);
      return result;
    }
    e = e->next;
  }
}

void
seek(int fd, unsigned position) {
  struct thread *cur = thread_current();
  struct list_elem *e = list_begin(&cur->file_fd_list);
  while (e != list_end(&cur->file_fd_list)){
    struct fileWithFd *fileFd = list_entry(e, struct fileWithFd, file_elem);
    if (fileFd->fd == fd ) {
      file_seek(fileFd->f, position);
    }
    e = e->next;
  }
}

unsigned
tell(int fd) {
  struct thread *cur = thread_current();
  struct list_elem *e = list_begin(&cur->file_fd_list);
  while (e != list_end(&cur->file_fd_list)){
    struct fileWithFd *fileFd = list_entry(e, struct fileWithFd, file_elem);
    if (fileFd->fd == fd ) {
      return (unsigned int) file_tell(fileFd->f);
    }
    e = e->next;
  }
}

void
close(int fd) {
  struct thread *cur = thread_current();
  struct list_elem *e = list_begin(&cur->file_fd_list);
  while (e != list_end(&cur->file_fd_list)){
    struct fileWithFd *fileFd = list_entry(e, struct fileWithFd, file_elem);
    if (fileFd->fd == fd ) {
      if (fileFd->f == NULL || fileFd->tid != cur->tid) {
        return;
      }
      file_close(fileFd->f);
      list_remove(e);
      e = e->next;
      free(fileFd);
    } else {
      e = e->next;
    }
  }
}