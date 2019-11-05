#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <user/syscall.h>
#include "devices/shutdown.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "process.h"
#include "file.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
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