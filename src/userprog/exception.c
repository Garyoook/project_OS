#include "userprog/exception.h"
#include <inttypes.h>
#include <stdio.h>
#include "filesys/file.h"
#include <string.h>
#include "threads/vaddr.h"
#include "userprog/gdt.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "syscall.h"
#include "vm/page.h"
#include "userprog/process.h"
#include <debug.h>
#include "userprog/pagedir.h"
#include "threads/palloc.h"
#include "vm/frame.h"
#include "vm/swap.h"

/* Number of page faults processed. */
static long long page_fault_cnt;

static void kill (struct intr_frame *);
static void page_fault (struct intr_frame *);

/* Registers handlers for interrupts that can be caused by user
   programs.

   In a real Unix-like OS, most of these interrupts would be
   passed along to the user process in the form of signals, as
   described in [SV-386] 3-24 and 3-25, but we don't implement
   signals.  Instead, we'll make them simply kill the user
   process.

   Page faults are an exception.  Here they are treated the same
   way as other exceptions, but this will need to change to
   implement virtual memory.

   Refer to [IA32-v3a] section 5.15 "Exception and Interrupt
   Reference" for a description of each of these exceptions. */
void
exception_init (void)
{
  /* These exceptions can be raised explicitly by a user program,
     e.g. via the INT, INT3, INTO, and BOUND instructions.  Thus,
     we set DPL==3, meaning that user programs are allowed to
     invoke them via these instructions. */
  intr_register_int (3, 3, INTR_ON, kill, "#BP Breakpoint Exception");
  intr_register_int (4, 3, INTR_ON, kill, "#OF Overflow Exception");
  intr_register_int (5, 3, INTR_ON, kill,
                     "#BR BOUND Range Exceeded Exception");

  /* These exceptions have DPL==0, preventing user processes from
     invoking them via the INT instruction.  They can still be
     caused indirectly, e.g. #DE can be caused by dividing by
     0.  */
  intr_register_int (0, 0, INTR_ON, kill, "#DE Divide Error");
  intr_register_int (1, 0, INTR_ON, kill, "#DB Debug Exception");
  intr_register_int (6, 0, INTR_ON, kill, "#UD Invalid Opcode Exception");
  intr_register_int (7, 0, INTR_ON, kill,
                     "#NM Device Not Available Exception");
  intr_register_int (11, 0, INTR_ON, kill, "#NP Segment Not Present");
  intr_register_int (12, 0, INTR_ON, kill, "#SS Stack Fault Exception");
  intr_register_int (13, 0, INTR_ON, kill, "#GP General Protection Exception");
  intr_register_int (16, 0, INTR_ON, kill, "#MF x87 FPU Floating-Point Error");
  intr_register_int (19, 0, INTR_ON, kill,
                     "#XF SIMD Floating-Point Exception");

  /* Most exceptions can be handled with interrupts turned on.
     We need to disable interrupts for page faults because the
     fault address is stored in CR2 and needs to be preserved. */
  intr_register_int (14, 0, INTR_OFF, page_fault, "#PF Page-Fault Exception");
}

/* Prints exception statistics. */
void
exception_print_stats (void)
{
  printf ("Exception: %lld page faults\n", page_fault_cnt);
}

/* Handler for an exception (probably) caused by a user process. */
static void
kill (struct intr_frame *f)
{
  /* This interrupt is one (probably) caused by a user process.
     For example, the process might have tried to access unmapped
     virtual memory (a page fault).  For now, we simply kill the
     user process.  Later, we'll want to handle page faults in
     the kernel.  Real Unix-like operating systems pass most
     exceptions back to the process via signals, but we don't
     implement them. */

  /* The interrupt frame's code segment value tells us where the
     exception originated. */
  switch (f->cs)
    {
    case SEL_UCSEG:
      /* User's code segment, so it's a user exception, as we
         expected.  Kill the user process.  */
      printf ("%s: dying due to interrupt %#04x (%s).\n",
              thread_name (), f->vec_no, intr_name (f->vec_no));
      intr_dump_frame (f);
      thread_exit ();

    case SEL_KCSEG:
      /* Kernel's code segment, which indicates a kernel bug.
         Kernel code shouldn't throw exceptions.  (Page faults
         may cause kernel exceptions--but they shouldn't arrive
         here.)  Panic the kernel to make the point.  */
      intr_dump_frame (f);
      PANIC ("Kernel bug - unexpected interrupt in kernel");

    default:
      /* Some other code segment?  Shouldn't happen.  Panic the
         kernel. */
      printf ("Interrupt %#04x (%s) in unknown segment %04x\n",
             f->vec_no, intr_name (f->vec_no), f->cs);
      thread_exit ();
    }
}

/* Page fault handler.  This is a skeleton that must be filled in
   to implement virtual memory.  Some solutions to task 2 may
   also require modifying this code.

   At entry, the address that faulted is in CR2 (Control Register
   2) and information about the fault, formatted as described in
   the PF_* macros in exception.h, is in F's error_code member.  The
   example code here shows how to parse that information.  You
   can find more information about both of these in the
   description of "Interrupt 14--Page Fault Exception (#PF)" in
   [IA32-v3a] section 5.15 "Exception and Interrupt Reference". */
//bool
//install_page (void *upage, void *kpage, bool writable)
//{
//  struct thread *t = thread_current ();
//
//  /* Verify that there's not already a page at that virtual
//     address, then map our page there. */
//  return (pagedir_get_page (t->pagedir, upage) == NULL
//          && pagedir_set_page (t->pagedir, upage, kpage, writable));
//}

static void
page_fault (struct intr_frame *f) {

  bool not_present;  /* True: not-present page, false: writing r/o page. */
  bool write;        /* True: access was write, false: access was read. */
  bool user;         /* True: access by user, false: access by kernel. */
  void *fault_addr;  /* Fault address. */

  /* Obtain faulting address, the virtual address that was
     accessed to cause the fault.  It may point to code or to
     data.  It is not necessarily the address of the instruction
     that caused the fault (that's f->eip).
     See [IA32-v2a] "MOV--Move to/from Control Registers" and
     [IA32-v3a] 5.15 "Interrupt 14--Page Fault Exception
     (#PF)". */
  asm ("movl %%cr2, %0" : "=r" (fault_addr));

  lock_acquire(&page_fault_lock);

  /* Turn interrupts back on (they were only off so that we could
     be assured of reading CR2 before it changed). */
  intr_enable ();
  /* Count page faults. */
  page_fault_cnt++;
  /* Determine cause. */
  not_present = (f->error_code & PF_P) == 0;
  write = (f->error_code & PF_W) != 0;
  user = (f->error_code & PF_U) != 0;
  uint8_t *upage = pg_round_down(fault_addr);
  if (!is_user_vaddr(fault_addr) || fault_addr == NULL || fault_addr >= PHYS_BASE
      || fault_addr < (void *) 0x08048000) {
    exit(EXIT_FAIL);
  }
  if (lookup_swap(upage) != NULL && lookup_swap(upage)->t_blongs_to == thread_current()) {
    if (pagedir_get_page(thread_current()->pagedir, upage) == NULL) {
      struct frame* frame = frame_create(PAL_USER, thread_current(), upage);
      install_page(upage, frame->kpage, true);
      read_from_swap(upage, frame->kpage);
      lock_release(&page_fault_lock);
      return;
    } else {
      read_from_swap(upage, pagedir_get_page(thread_current()->pagedir, upage));
    }
  }

  struct spage* s_page =  lookup_spage(upage);

  if (s_page == NULL) {
    if (fault_addr >= f->esp - 32 && !for_stack_growth) {
    bool success = false;
    struct frame* frame = frame_create(PAL_USER, thread_current(),
        PHYS_BASE - num * PGSIZE);
    if (frame->kpage != NULL) {
      /* a stack limit of 8 MB */
      if (num > 2048) {
        exit(EXIT_FAIL);
      }
      success = install_page(((uint8_t *) PHYS_BASE) - num * PGSIZE,
          frame->kpage, true);
      if (success) {
        struct spage *s = create_spage(NULL, 0,
            ((uint8_t *) PHYS_BASE) - num * PGSIZE, 0, 0, false);
        s->kpage = frame->kpage;   //sharing
        num++;
        thread_current()->stack = PHYS_BASE - PGSIZE;
      } else {
        palloc_free_page(frame->kpage);
        exit(EXIT_FAIL);
      }
    }
      lock_release(&page_fault_lock);
      return;
    } else {
      exit(EXIT_FAIL);
    }

  } else {
    uint32_t read_bytes = s_page->read_bytes;
    uint32_t zero_bytes = s_page->zero_bytes;
    uint8_t *upage_grow = s_page->upage;
    off_t ofs = s_page->offset;
    bool writable = s_page->writable;
    if (not_present) {
      file_seek(s_page->file_sp, ofs);
      while (read_bytes > 0 || zero_bytes > 0) {
        /* Calculate how to fill this page.
           We will read PAGE_READ_BYTES bytes from FILE
           and zero the final PAGE_ZERO_BYTES bytes. */
        size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
        size_t page_zero_bytes = PGSIZE - page_read_bytes;

        struct frame *frame = frame_create(PAL_USER,
            thread_current(), upage_grow);
        s_page->kpage = frame->kpage;
        s_page->has_load_in = true;

        if (frame->kpage == NULL)
        {
          exit(EXIT_FAIL);
        }
        s_page->kpage = frame->kpage; // sgaring

        /* Load this page. */
        if (file_read(s_page->file_sp, frame->kpage,
            page_read_bytes) != (int) page_read_bytes) {
          palloc_free_page(frame->kpage);
          exit(EXIT_FAIL);
        }
        memset(frame->kpage + page_read_bytes, 0, page_zero_bytes);

        /* Add the page to the process's address space. */
        if (!install_page(upage_grow, frame->kpage, writable)) {
          palloc_free_page(frame->kpage);
          exit(EXIT_FAIL);
        }

        /* Advance. */
        read_bytes -= page_read_bytes;
        zero_bytes -= page_zero_bytes;
        upage_grow += PGSIZE;
      }
      lock_release(&page_fault_lock);
      return;
    }

  }

  if (thread_current()->in_syscall) {
    exit(EXIT_FAIL);
  } else if (!user) {
    kill(f);
  }

  /* To implement virtual memory, delete the rest of the function
     body, and replace it with code that brings in the page to
     which fault_addr refers. */
  printf ("Page fault at %p: %s error %s page in %s context.\n",
          fault_addr,
          not_present ? "not present" : "rights violation",
          write ? "writing" : "reading",
          user ? "user" : "kernel");
  kill (f);
}

