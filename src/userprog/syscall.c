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
    const char *name;
    int fd;
    struct file *f;
    struct list_elem file_elem;
    tid_t tid;
    bool reopend;
};


//struct fileWithFd fileFdArray[FILE_LIMIT];
//tid_t tidArray[FILE_LIMIT];
//bool reopen[FILE_LIMIT];
int currentFd = STD_IO;


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
  t->in_syscall = true;
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

void
halt(void) {
  shutdown_power_off();
}

void
exit (int status) {

  struct thread *cur = thread_current();

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


//    enum intr_level old_level = intr_disable();
//
//    struct list_elem *e1 = list_begin(&cur->child_list);
//    while (e1 != list_end(&cur->child_list)) {
//      struct child *thr_child = list_entry(e1, struct child, child_elem);
//      printf("**** tid = %d", thr_child->tid);
//      struct thread *t = lookup_tid(thr_child->tid);
//      if (t != NULL) {
//        t->parent = NULL;
//      }
//      e1 = list_next(e1);
//    }
//    intr_set_level(old_level);
  }


//  struct list_elem *e = list_begin(&cur->child_list);
//  while (e != list_end(&cur->child_list)){
//    struct child *thr_child = list_entry(e, struct child, child_elem);
//    list_remove(e);
//    free(thr_child);
//    e = e->next;
//  }
//
//  struct list_elem *ew = list_begin(&cur->child_wait_list);
//  while (ew != list_end(&cur->child_wait_list)){
//    struct child *thr_child = list_entry(ew, struct child, child_elem);
//    list_remove(ew);
//    free(thr_child);
//    ew = ew->next;
//  }

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
//  enum intr_level old_level;
//  old_level = intr_disable();

//  struct thread *cur = thread_current();
//
//  struct list_elem *e = list_begin(&cur->child_wait_list);
//  while (e != list_end(&cur->child_wait_list)){
//    struct child *thr_child = list_entry(e, struct child, child_elem);
//    if (thr_child->tid == pid) {
//      sema_down(&thr_child->child_sema);
//      list_remove(e);
////      free(thr_child);
//      return thr_child->exit_status;
//    }
//    e = e->next;
//  }

  int result = process_wait(pid);

//  struct list_elem *ew= list_begin(&cur->child_list);
//  while (ew!= list_end(&cur->child_list)){
//    struct child *thr_child = list_entry(ew, struct child, child_elem);
//    list_remove(ew);
//    free(thr_child);
//    ew= ew->next;
//  }

//  intr_set_level(old_level);

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
  struct thread *cur = thread_current();

  if (!strcmp(file, cur->name)) {
    reopened = true;
  }
  if (!strcmp(file, current_file_name)) {
    reopened = true;
  }

//  for (int i = 0; i < FILE_LIMIT; i++) {
//    if (fileFdArray[i].f != NULL) {
//      if (!strcmp(fileFdArray[i].name, file)) {
//        reopened = true;
//        file1 = file_reopen(fileFdArray[i].f);
//        break;
//      }
//    }
//  }

  if (strlen(file) == 0 || file1 == NULL) {
    return EXIT_FAIL;
  }

  if (currentFd <= FILE_LIMIT) {
    lock_acquire(&wait_lock);
    currentFd++;
    lock_release(&wait_lock);

//    struct list_elem *e = list_begin(&cur->file_fd_list);
      struct fileWithFd *fileFd = (struct fileWithFd *) malloc(sizeof(struct fileWithFd));
      ASSERT(fileFd);
      fileFd->name = file;
      fileFd->fd = currentFd;
      fileFd->f = file1;
      fileFd->tid = cur->tid;
      fileFd->reopend = reopened;
      if (fileFd->reopend) {
        file_reopen(fileFd->f);
      }

      list_push_back(&cur->file_fd_list, &fileFd->file_elem);

  } else {
    return EXIT_FAIL;
  }

//  if (currentFd <= FILE_LIMIT) {
//    fileFdArray[currentFd-STD_IO].name = file;
//    fileFdArray[currentFd-STD_IO].fd = currentFd;
//    fileFdArray[currentFd-STD_IO].f = file1;
//    tidArray[currentFd-STD_IO] = thread_current()->tid;
//    reopen[currentFd-STD_IO] = reopened;
//  } else {
//    return EXIT_FAIL;
//  }

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

//  if (fileFdArray[fd-STD_IO].f == NULL) {
//    exit(EXIT_FAIL);
//  }
//  return file_length(fileFdArray[fd-STD_IO].f);
}

int
read(int fd, void *buffer, unsigned size) {
  if (fd < STDOUT_FILENO || fd > FILE_LIMIT) {
    exit(EXIT_FAIL);
  }
//  if (tidArray[fd-STD_IO] != thread_current()->tid) {
//    exit(EXIT_FAIL);
//  }
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

//  struct file *currentFile = fileFdArray[fd-STD_IO].f;
//
//  if (currentFile != NULL) {
//    int id = file_read(currentFile, buffer, size);
//    return id;
//  } else {
//    exit(EXIT_FAIL);
//  }
}

int
write(int fd, const void *buffer, unsigned size) {
  if (fd < STDOUT_FILENO || fd > FILE_LIMIT) {
    exit(EXIT_FAIL);
  }
  if (fd == STDOUT_FILENO) {
    // size may not bigger than hundred bytes
    // otherwise may confused
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
        if (!fileFd->reopend)
          file_allow_write(fileFd->f);
      }
      int result = file_write(fileFd->f, buffer, size);
      file_deny_write(fileFd->f);
      return result;
    }
    e = e->next;
  }




//  if (tidArray[fd-STD_IO] != thread_current()->tid || reopen[fd-STD_IO]) {
////    exit(EXIT_FAIL);
//  } else {
//    file_allow_write(fileFdArray[fd-STD_IO].f);
//  }
//
//  int result = file_write(fileFdArray[fd-STD_IO].f, buffer, size);
//
//  file_deny_write(fileFdArray[fd-STD_IO].f);
//  return result;
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

//  file_seek(fileFdArray[fd-STD_IO].f, position);
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

//  return (unsigned int) file_tell(fileFdArray[fd-STD_IO].f);
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
      if (fd <= FILE_LIMIT) {
        file_close(fileFd->f);
        list_remove(e);
//        free(fileFd);
      }
    }
    e = e->next;
  }

//  if (fileFdArray[fd-STD_IO].f == NULL){
//    return;
//  }
//  if (tidArray[fd-STD_IO] != thread_current()->tid) {
//    return;
//  }
//  if (fd <= FILE_LIMIT) {
//    file_close(fileFdArray[fd-STD_IO].f);
//    fileFdArray[fd-STD_IO].f = NULL;
//  }
}