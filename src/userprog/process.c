#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <string.h>
#include <kernel/hash.h>
#include <vm/page.h>
#include "vm/swap.h"
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "syscall.h"
#include "vm/frame.h"

// a macro to reduce duplicated code in arguments passing;
#define \
  PUSH_STACK(esp, from, size) \
    { \
        for_stack_growth = false;\
        esp = esp - size;\
        memcpy(esp, from, size);\
        bytes_used += size;\
        check_stack_overflow(bytes_used);\
        for_stack_growth = true;\
    };

static thread_func start_process NO_RETURN;
static bool load (const char *cmdline, void (**eip) (void), void **esp);


/* Starts a new thread running a user program loaded from
   FILENAME.  The new thread may be scheduled (and may even exit)
   before process_execute() returns.  Returns the new process's
   thread id, or TID_ERROR if the thread cannot be created. */
tid_t
process_execute (const char *file_name)
{
char *fn_copy;
  tid_t tid;

  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */
  fn_copy = palloc_get_page (0);
  if (fn_copy == NULL)
    return TID_ERROR;
  strlcpy (fn_copy, file_name, PGSIZE);

  /* Create a new thread to execute FILE_NAME. */
  char command_name[FILE_NAME_LEN_LIMIT];
  char file_name_copy[FILE_NAME_LEN_LIMIT];
  char *save_ptr;

  // separate the executable name with the arguments, and then
  // pass it to file_open and thread_create;
  strlcpy(file_name_copy, file_name, FILE_NAME_LEN_LIMIT);
  strlcpy(command_name, strtok_r((char *) file_name_copy, " ", &save_ptr),
      FILE_NAME_LEN_LIMIT);

  // add lock to prevent synchronization problems in filesys operation.
  lock_acquire(&syscall_lock);
  struct file *file = filesys_open(command_name);
  lock_release(&syscall_lock);

  // check command_name file exists
  if (file == NULL) {
    palloc_free_page(fn_copy);
    return EXIT_FAIL;
  }
  file_close(file);

  struct child *child = (struct child *) malloc(sizeof(struct child));
  if (child == NULL) {
    palloc_free_page(fn_copy);
    return EXIT_FAIL;
  }
  sema_init(&child->child_sema, 0);
  sema_init(&thread_current()->child_entry_sema, 0);
  sema_init(&thread_current()->child_load_sema, 0);

  tid = thread_create (command_name, PRI_DEFAULT, start_process, fn_copy);

  sema_down(&thread_current()->child_load_sema);
  // if the child do not get loaded properly, free it and exit with code -1;
  if (!thread_current()->load_success) {
    free(child);
    return EXIT_FAIL;
  }

  if (tid == TID_ERROR) {
    palloc_free_page (fn_copy);
    free(child);
    return EXIT_FAIL;
  }

  // set the fields of the child process and push it into the child list.
  child->tid = tid;
  child->exit_status = -1;
  list_push_back(&thread_current()->child_list, &child->child_elem);
  // inform the parent that the child has been added into the list.
  sema_up(&thread_current()->child_entry_sema);

  return tid;
}

// a newly added function to check the stack is not overflowed.
static void check_stack_overflow(int used);

// use this to pass and push the arguments to the stack:
static bool argument_passing(void **esp, char *file_name) {
  current_file_name = malloc(strlen(file_name) + 1);
// push arguments to the stack;
  enum intr_level old_level;
  old_level = intr_disable();

  size_t cmdLen = strlen(file_name);
  char s[cmdLen];
  strlcpy(s, file_name, (cmdLen + 1));

  // keep checking this to ensure the arguments do not exceed a single page;
  int bytes_used = 0;

  char *token, *save_ptr;
  int argc= 0;

  char *argArr[ARGC];
  void *addrArr[ARGC];
  void *addr_argv;

  // tokenize the arguments and store them into the argArr.
  for (token = strtok_r (s, " ", &save_ptr); token != NULL;
       token = strtok_r (NULL, " ", &save_ptr)) {
    argArr[argc] = token;
    argc++;
  }

  // push the arguments to the stack;
  for (int i = argc - 1; i >= 0; i--) {
    PUSH_STACK(*esp, argArr[i], (strlen(argArr[i]) + 1));
    addrArr[i] = *esp;
  }

  // Aligning the esp to a nearest multiple of 4
  int zero = 0;
  void *addr = (void *) ROUND_DOWN ((uint64_t) *esp , 4);
  size_t addr_diff = ((size_t)*esp - (size_t)addr);
  PUSH_STACK(*esp, &zero, addr_diff);

  //push sentinel to the stack
  PUSH_STACK(*esp, &zero, sizeof(char *))

  // push the address of arguments to the stack:
  for (int i = argc - 1; i >= 0; i--) {
    PUSH_STACK(*esp, &addrArr[i], sizeof(char *));
  }
  addr_argv = *esp;

  // push address of the command name
  PUSH_STACK(*esp, &addr_argv, sizeof(char **));

  //push the argc to the stack:
  *esp = *esp - sizeof (int);
  *(int *)*esp = argc;

  // last, the null pointer to the stack.

  void *nullPtr = NULL;
  PUSH_STACK(*esp, &nullPtr, sizeof(void *));

  intr_set_level(old_level);

  return true;
}

// check that the bytes used does not exceed the limit of stack.
static void check_stack_overflow(int used) {
  if (used > STACK_LIMIT) {
    free(current_file_name);
    exit(EXIT_FAIL);
  }
}

/* A thread function that loads a user process and starts it
  running. */
static void
start_process (void *file_name_)
{
  char *file_name = file_name_;
  struct intr_frame if_;
  struct thread *cur = thread_current();
  bool success;
  hash_init(&cur->spage_table , &page_hash, &page_less, NULL);
  list_init(&swap_table);
  /* Initialize interrupt frame and load executable. */
  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;

  size_t cmdLen = strlen(file_name);
  char s[cmdLen];
  strlcpy(s, file_name, (cmdLen + 1));

  // take the executable name and pass it to load();
  char *command_name, *save_ptr;
  command_name = strtok_r((char *) s, " ", &save_ptr);
  init_swap_block();
  success = load (command_name, &if_.eip, &if_.esp);

  cur->parent->load_success = success;
  sema_up(&cur->parent->child_load_sema);


  // if load succeeded we start passing the arguments to the stack:
  if (success) {
    success = argument_passing(
        &if_.esp, file_name);
  }

  /* If load failed, quit. */
  palloc_free_page (file_name);

  if (!success)
    thread_exit ();

  sema_down(&thread_current()->parent->child_entry_sema);

  /* Start the user process by simulating a return from an
     interrupt, implemented by intr_exit (in
     threads/intr-stubs.S).  Because intr_exit takes all of its
     arguments on the stack in the form of a `struct intr_frame',
     we just point the stack pointer (%esp) to our stack frame
     and jump to it. */
  asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
  NOT_REACHED ();
}

/* Waits for thread TID to die and returns its exit status.  If
   it was terminated by the kernel (i.e. killed due to an
   exception), returns -1.  If TID is invalid or if it was not a
   child of the calling process, or if process_wait() has already
   been successfully called for the given TID, returns -1
   immediately, without waiting.

   This function will be implemented in problem 2-2.  For now, it
   does nothing. */
int
process_wait (tid_t child_tid)
{
  enum intr_level old_level;
  old_level = intr_disable();

  struct thread *cur = thread_current();
  struct list_elem *e = list_begin(&cur->child_list);

  while (e != list_end(&cur->child_list)){
    struct child *thr_child = list_entry(e, struct child, child_elem);
    // if the wait target child is in the list, sema down
    // and remove it from the list. return the exit status of the child.
    if (thr_child->tid == child_tid) {
      sema_down(&thr_child->child_sema);
      list_remove(e);

      return thr_child->exit_status;
    }
    e = e->next;
  }
  intr_set_level(old_level);

  return EXIT_FAIL;
}

/* Free the current process's resources. */
void
process_exit (void)
{
  struct thread *cur = thread_current ();
  uint32_t *pd;

  /* Destroy the current process's page directory and switch back
     to the kernel-only page directory. */
  pd = cur->pagedir;
  if (pd != NULL)
    {
      // free the name we malloced for checking file reopen;
      if (thread_current()->name == current_file_name)
        free(current_file_name);
      /* Correct ordering here is crucial.  We must set
         cur->pagedir to NULL before switching page directories,
         so that a timer interrupt can't switch back to the
         process page directory.  We must activate the base page
         directory before destroying the process's page
         directory, or our active page directory will be one
         that's been freed (and cleared). */
      cur->pagedir = NULL;
      pagedir_activate (NULL);
      pagedir_destroy (pd);
    }
}

/* Sets up the CPU for running user code in the current
   thread.
   This function is called on every context switch. */
void
process_activate (void)
{
  struct thread *t = thread_current ();

  /* Activate thread's page tables. */
  pagedir_activate (t->pagedir);

  /* Set thread's kernel stack for use in processing
     interrupts. */
  tss_update ();
}

/* We load ELF binaries.  The following definitions are taken
   from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* For use with ELF types in printf(). */
#define PE32Wx PRIx32   /* Print Elf32_Word in hexadecimal. */
#define PE32Ax PRIx32   /* Print Elf32_Addr in hexadecimal. */
#define PE32Ox PRIx32   /* Print Elf32_Off in hexadecimal. */
#define PE32Hx PRIx16   /* Print Elf32_Half in hexadecimal. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
   This appears at the very beginning of an ELF binary. */
struct Elf32_Ehdr
  {
    unsigned char e_ident[16];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
  };

/* Program header.  See [ELF1] 2-2 to 2-4.
   There are e_phnum of these, starting at file offset e_phoff
   (see [ELF1] 1-6). */
struct Elf32_Phdr
  {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
  };

/* Values for p_type.  See [ELF1] 2-3. */
#define PT_NULL    0            /* Ignore. */
#define PT_LOAD    1            /* Loadable segment. */
#define PT_DYNAMIC 2            /* Dynamic linking info. */
#define PT_INTERP  3            /* Name of dynamic loader. */
#define PT_NOTE    4            /* Auxiliary info. */
#define PT_SHLIB   5            /* Reserved. */
#define PT_PHDR    6            /* Program header table. */
#define PT_STACK   0x6474e551   /* Stack segment. */

/* Flags for p_flags.  See [ELF3] 2-3 and 2-4. */
#define PF_X 1          /* Executable. */
#define PF_W 2          /* Writable. */
#define PF_R 4          /* Readable. */

bool setup_stack(void **esp);
static bool validate_segment (const struct Elf32_Phdr *, struct file *);
static bool load_segment (struct file *file, off_t ofs, uint8_t *upage,
                          uint32_t read_bytes, uint32_t zero_bytes,
                          bool writable);


/* Loads an ELF executable from FILE_NAME into the current thread.
   Stores the executable's entry point into *EIP
   and its initial stack pointer into *ESP.
   Returns true if successful, false otherwise. */

bool
load (const char *cmdline, void (**eip) (void), void **esp)
{
  struct thread *t = thread_current ();
  struct Elf32_Ehdr ehdr;
  struct file *file = NULL;
  off_t file_ofs;
  bool success = false;
  int i;

  /* Allocate and activate page directory. */
  t->pagedir = pagedir_create ();
  if (t->pagedir == NULL)
    goto done;
  process_activate ();

  /* Open executable file. */
  lock_acquire(&syscall_lock);
  file = filesys_open (cmdline);
  lock_release(&syscall_lock);
  if (file == NULL)
    {
      printf ("load: %s: open failed\n", cmdline);
      goto done;
    }

  /* Read and verify executable header. */
  if (file_read (file, &ehdr, sizeof ehdr) != sizeof ehdr
      || memcmp (ehdr.e_ident, "\177ELF\1\1\1", 7)
      || ehdr.e_type != 2
      || ehdr.e_machine != 3
      || ehdr.e_version != 1
      || ehdr.e_phentsize != sizeof (struct Elf32_Phdr)
      || ehdr.e_phnum > 1024)
    {
      printf ("load: %s: error loading executable\n", cmdline);
      goto done;
    }

  /* Read program headers. */
  file_ofs = ehdr.e_phoff;
  for (i = 0; i < ehdr.e_phnum; i++)
    {
      struct Elf32_Phdr phdr;

      if (file_ofs < 0 || file_ofs > file_length (file))
        goto done;
      file_seek (file, file_ofs);

      if (file_read (file, &phdr, sizeof phdr) != sizeof phdr)
        goto done;
      file_ofs += sizeof phdr;
      switch (phdr.p_type)
        {
        case PT_NULL:
        case PT_NOTE:
        case PT_PHDR:
        case PT_STACK:
        default:
          /* Ignore this segment. */
          break;
        case PT_DYNAMIC:
        case PT_INTERP:
        case PT_SHLIB:
          goto done;
        case PT_LOAD:
          if (validate_segment (&phdr, file))
            {
              bool writable = (phdr.p_flags & PF_W) != 0;
              uint32_t file_page = phdr.p_offset & ~PGMASK;
              uint32_t mem_page = phdr.p_vaddr & ~PGMASK;
              uint32_t page_offset = phdr.p_vaddr & PGMASK;
              uint32_t read_bytes, zero_bytes;
              if (phdr.p_filesz > 0)
                {
                  /* Normal segment.
                     Read initial part from disk and zero the rest. */
                  read_bytes = page_offset + phdr.p_filesz;
                  zero_bytes = (ROUND_UP (page_offset + phdr.p_memsz, PGSIZE)
                                - read_bytes);
                }
              else
                {
                  /* Entirely zero.
                     Don't read anything from disk. */
                  read_bytes = 0;
                  zero_bytes = ROUND_UP (page_offset + phdr.p_memsz, PGSIZE);
                }
              if (!load_segment (file, file_page, (void *) mem_page,
                                 read_bytes, zero_bytes, writable))
                goto done;
            }
          else
            goto done;
          break;
        }
    }

  /* Set up stack. */
  if (!setup_stack(esp))
    goto done;

  /* Start address. */
  *eip = (void (*) (void)) ehdr.e_entry;

  success = true;

  // deny write as the file is ready to run at the moment.
  file_deny_write (file);
 done:
  /* We arrive here whether the load is successful or not. */
  file_close (file);
  return success;
}

/* load() helpers. */



/* Checks whether PHDR describes a valid, loadable segment in
   FILE and returns true if so, false otherwise. */
static bool
validate_segment (const struct Elf32_Phdr *phdr, struct file *file)
{
  /* p_offset and p_vaddr must have the same page offset. */
  if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK))
    return false;

  /* p_offset must point within FILE. */
  if (phdr->p_offset > (Elf32_Off) file_length (file))
    return false;

  /* p_memsz must be at least as big as p_filesz. */
  if (phdr->p_memsz < phdr->p_filesz)
    return false;

  /* The segment must not be empty. */
  if (phdr->p_memsz == 0)
    return false;

  /* The virtual memory region must both start and end within the
     user address space range. */
  if (!is_user_vaddr ((void *) phdr->p_vaddr))
    return false;
  if (!is_user_vaddr ((void *) (phdr->p_vaddr + phdr->p_memsz)))
    return false;

  /* The region cannot "wrap around" across the kernel virtual
     address space. */
  if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
    return false;

  /* Disallow mapping page 0.
     Not only is it a bad idea to map page 0, but if we allowed
     it then user code that passed a null pointer to system calls
     could quite likely panic the kernel by way of null pointer
     assertions in memcpy(), etc. */
  if (phdr->p_vaddr < PGSIZE)
    return false;

  /* It's okay. */
  return true;
}

/* Loads a segment starting at offset OFS in FILE at address
   UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
   memory are initialized, as follows:

        - READ_BYTES bytes at UPAGE must be read from FILE
          starting at offset OFS.

        - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.

   The pages initialized by this function must be writable by the
   user process if WRITABLE is true, read-only otherwise.

   Return true if successful, false if a memory allocation error
   or disk read error occurs. */
static bool
load_segment (struct file *file, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable)
{
  ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
  ASSERT (pg_ofs (upage) == 0);
  ASSERT (ofs % PGSIZE == 0);

  struct spage *s = create_spage(file, ofs, upage, read_bytes, zero_bytes, writable);
  file_seek (file, ofs);
  while (read_bytes > 0 || zero_bytes > 0)
    {
      /* Calculate how to fill this page.
         We will read PAGE_READ_BYTES bytes from FILE
         and zero the final PAGE_ZERO_BYTES bytes. */
      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;

      /* Get a page of memory. */
      struct frame * f = frame_create(PAL_USER, thread_current(), upage);
      uint8_t *kpage = f->kpage;
      if (kpage == NULL)
        return false;

      s->kpage = kpage;
      /* Load this page. */
      if (file_read (file, kpage, page_read_bytes) != page_read_bytes)
        {
          palloc_free_page (kpage);
          return false;
        }
      memset (kpage + page_read_bytes, 0, page_zero_bytes);

      /* Add the page to the process's address space. */
      if (!install_page (upage, kpage, writable))
        {
          palloc_free_page (kpage);
          return false;
        }

      /* Advance. */
      read_bytes -= page_read_bytes;
      zero_bytes -= page_zero_bytes;
      upage += PGSIZE;
    }
  return true;
}

/* Create a minimal stack by mapping a zeroed page at the top of
   user virtual memory. */
bool
setup_stack(void **esp)
{
  uint8_t *kpage;
  bool success = false;

  struct frame *f = frame_create(PAL_ZERO | PAL_USER, thread_current(), PHYS_BASE - PGSIZE);
//      palloc_get_page (PAL_USER | PAL_ZERO);
  if (f->kpage != NULL)
    {
      success = install_page (((uint8_t *) PHYS_BASE) - PGSIZE, f->kpage, true);

      if (success) {
        struct spage *s = create_spage(NULL, 0, ((uint8_t *) PHYS_BASE) - PGSIZE, 0, 0, false);
        s->kpage = kpage;
        *esp = PHYS_BASE;
      } else {
        palloc_free_page(f->kpage);
        return success;
      }
    }

  return success;
}

/* Adds a mapping from user virtual address UPAGE to kernel
   virtual address KPAGE to the page table.
   If WRITABLE is true, the user process may modify the page;
   otherwise, it is read-only.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   Returns true on success, false if UPAGE is already mapped or
   if memory allocation fails. */
bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable));
}
