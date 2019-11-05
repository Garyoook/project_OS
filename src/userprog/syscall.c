#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <user/syscall.h>
#include "devices/shutdown.h"
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f)
{
  void **stack = f->esp;
  int *syscallNum = *stack;
  *stack = *stack + sizeof(int *);
  char *args[8];
  int i = 0;
  while (*stack != (uint8_t) 0) {
    args[i] = *stack;
    *stack = *stack + sizeof(args[i]);
    i++;
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

}

pid_t
exec(const char *cmd_line) {

}

int
wait(pid_t pid) {

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
write(int fd, void *buffer, unsigned size) {

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