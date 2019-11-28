#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <user/syscall.h>
#include <string.h>
#include "vm/frame.h"
#include "threads/palloc.h"
#include "vm/page.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "pagedir.h"
#include "threads/synch.h"
#include "devices/input.h"
#include "threads/malloc.h"
#include "process.h"

#define FILE_LIMIT 128
#define FD_INIT 2
#define MD_INIT 0

static void syscall_handler (struct intr_frame *);

// note that we did not use release_all_locks() because we have
// dealt with all the locks in exit(), keep using it will cause kernel panic;
void release_all_locks(struct thread *t);

// a struct to keep track of a file with its name, fd, tid and open status.
struct fileWithFd {
  struct file *f;
  const char *name;
  int fd;

  int md;
  void *md_addr;
  int file_size;
  off_t file_pos;
  tid_t tid;
  bool mmaped;
  bool reopened;
  struct list_elem file_elem;
};

// the initial file descriptor number;
int currentFd = FD_INIT;
int currentMd = MD_INIT;

void
syscall_init (void)
{
  lock_init(&syscall_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

// for user access memory
bool safe_access(void const *esp) {
  return (is_user_vaddr(esp) &&
    pagedir_get_page(thread_current()->pagedir, esp) &&
    esp != NULL);
}

void exit_if_unsafe(void const *esp) {
  if (!safe_access(esp)) {
    exit(EXIT_FAIL);
  }
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

//  printf("IIIIIIIIIIIII: %d\n", syscall_num);
  switch (syscall_num) {
    case SYS_EXEC:
      if (!safe_access(fst)) f->eax = (uint32_t) EXIT_FAIL;
      f->eax = (uint32_t) exec(*(char **)fst);
      break;
    case SYS_CLOSE:
      exit_if_unsafe(fst);
      close(*(int *)fst);
      break;
    case SYS_CREATE:
      exit_if_unsafe(fst);
      exit_if_unsafe(snd);
      f->eax = (uint32_t) create(*(char **)fst, (void *)*(int *)snd);
      break;
    case SYS_EXIT:
      exit_if_unsafe(fst);
      exit(*(int*)fst);
      break;
    case SYS_FILESIZE:
      exit_if_unsafe(fst);
      f->eax = (uint32_t) filesize(*(int *) fst);
      break;
    case SYS_HALT:
      halt();
    case SYS_OPEN:
      exit_if_unsafe(fst);
      exit_if_unsafe(*(char **)fst);
      f->eax = (uint32_t) open(*(char **)fst);
      break;
    case SYS_READ:
      exit_if_unsafe(fst);
      exit_if_unsafe(snd);
      exit_if_unsafe(*snd);
      exit_if_unsafe(trd);
      f->eax = (uint32_t) read(*(int *)fst, *snd, *(unsigned *) trd);
      break;
    case SYS_WRITE:
      exit_if_unsafe(fst);
      exit_if_unsafe(snd);
      exit_if_unsafe(*snd);
      exit_if_unsafe(trd);
      f->eax = (uint32_t) write(*(int *)fst, *snd, *(unsigned *) trd);
      break;
    case SYS_WAIT:
      exit_if_unsafe(fst);
      f->eax = (uint32_t) wait(*(pid_t *)fst);
      break;
    case SYS_REMOVE:
      exit_if_unsafe(fst);
      exit_if_unsafe(*(char **)fst);
      f->eax = (uint32_t) remove(*(const char **) fst);
      break;
    case SYS_SEEK:
      exit_if_unsafe(fst);
      exit_if_unsafe(snd);
      seek(*(int *) fst, *(unsigned int *) snd);
      break;
    case SYS_TELL:
      exit_if_unsafe(fst);
      f->eax = tell(*(int *) fst);
      break;
    case SYS_MMAP:
      exit_if_unsafe(fst);
      exit_if_unsafe(snd);
      f->eax = (uint32_t)mmap(*(int *)fst, *snd);
      break;
    case SYS_MUNMAP:
      exit_if_unsafe(fst);
      munmap(*(int *)fst);
      break;
    default:break;
  }
}

pid_t
exec(const char *cmd_line) {
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
  lock_acquire(&syscall_lock);
  struct file *file1 = filesys_open(file);
  lock_release(&syscall_lock);
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
    lock_acquire(&syscall_lock);
    currentFd++;

    // initialize the fields of a created file and push it into
    // the file list of current thread.
    struct fileWithFd *fileFd =
        (struct fileWithFd *) malloc(sizeof(struct fileWithFd));
    if (fileFd == NULL) {
      lock_release(&syscall_lock);
      return EXIT_FAIL;
    }
    fileFd->name = file;
    fileFd->fd = currentFd;
    lock_release(&syscall_lock);
    fileFd->f = file1;
    fileFd->tid = cur->tid;
    fileFd->reopened = reopened;
    fileFd->mmaped =false;
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
  return 0;
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

  return 0;
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

      int result;
      if (fileFd->mmaped) {
        uint32_t *kaddr = pagedir_get_page(cur->pagedir, fileFd->md_addr);
        //printf("Q\n");
        memcpy(kaddr + fileFd->file_pos, buffer, size);
        //printf("W\n");
        // may overflow
        return size;
      } else {
        result = file_write(fileFd->f, buffer, size);
      }
      file_deny_write(fileFd->f);

      return result;
    }
    e = e->next;
  }
  return -1;
}

void
seek(int fd, unsigned position) {
  struct thread *cur = thread_current();
  struct list_elem *e = list_begin(&cur->file_fd_list);
  while (e != list_end(&cur->file_fd_list)){
    struct fileWithFd *fileFd = list_entry(e, struct fileWithFd, file_elem);
    if (fileFd->fd == fd ) {
      if (fileFd->mmaped) {
        fileFd->file_pos = position;
      } else {
        file_seek(fileFd->f, position);
      }
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
      if (fileFd->mmaped) {
        return (unsigned)fileFd->file_pos;
      } else {
        return (unsigned int) file_tell(fileFd->f);
      }
    }
    e = e->next;
  }
  return 0;
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

mapid_t mmap(int fd, void *addr) {
  int file_size = filesize(fd);
  if (fd == 0 || fd == 1 || file_size == 0 || addr == 0 || (uint32_t) addr % PGSIZE != 0) {
    return -1;
  }

  int page_no = ((uint32_t)file_size) / PGSIZE;
  uint32_t zero_set = ((uint32_t)file_size)  % PGSIZE;
  off_t file_read_byte;

  // to check no overlapping
  for (int a = 0; a < page_no; a++) {
    if (pagedir_get_page(thread_current()->pagedir, addr + (uint32_t )(page_no * PGSIZE)) != NULL){
        //lookup_page((uint32_t *) addr + a) != NULL) {
      //?
      return -1;
    }
  }

  void *kaddr = palloc_get_page(PAL_USER);
  if (kaddr == NULL)
    return -1;

  struct thread *cur = thread_current();
  struct list_elem *e = list_begin(&cur->file_fd_list);
  while (e != list_end(&cur->file_fd_list)) {
    struct fileWithFd *fileFd = list_entry(e, struct fileWithFd, file_elem);
    if (fileFd->fd == fd) {
      if (fileFd->f == NULL || fileFd->tid != cur->tid) {
        return -1;
      }

      page_create(kaddr, fileFd->f, ALL_ZERO, false, 0);
      file_read_byte = file_read(fileFd->f, kaddr, file_size);

      if (file_read_byte != file_size) {
        palloc_free_page(kaddr);
        return -1;
      }

      if (zero_set != 0) {
        //find the address of ending of the file
        memset(kaddr + file_size, 0, (size_t) PGSIZE - zero_set);
      }

      if ( !(pagedir_get_page (cur->pagedir, addr) == NULL
            && pagedir_set_page (cur->pagedir, addr, kaddr, false))) {
        palloc_free_page(kaddr);
        return -1;
      }
      addr += PGSIZE;

      fileFd->md = currentMd;
      fileFd->md_addr = addr;
      fileFd->mmaped = true;
      currentMd++;
      fileFd->file_size = file_size;
      fileFd->file_pos = file_tell(fileFd->f);
      return fileFd->md;
    }
      e = e -> next;
  }
  return -1;
}

void munmap(mapid_t mapping)
{
  struct thread *cur = thread_current();
  struct list_elem *e = list_begin(&cur->file_fd_list);
  while (e != list_end(&cur->file_fd_list)) {
    struct fileWithFd *fileFd = list_entry(e, struct fileWithFd, file_elem);
    if (fileFd->md == mapping) {
      if (fileFd->f == NULL || fileFd->tid != cur->tid) {
        return;
      }
      if (fileFd->md_addr == NULL)
        return;

      if (!fileFd->reopened)
        file_allow_write(fileFd->f);

      off_t r = file_write_at(fileFd->f, pagedir_get_page(cur->pagedir, fileFd->md_addr), fileFd->file_size, 0);
      pagedir_clear_page(cur->pagedir, fileFd->md_addr);
      page_destroy(fileFd->md_addr);
    }
    e = e->next;
  }
}

void grow_stack(void * vaddr) {
  page_create(vaddr, NULL, ALL_ZERO, true, 0);

}