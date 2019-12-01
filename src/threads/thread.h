#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "fixed-point.h"
#include "synch.h"
#include "kernel/hash.h"

/* List of all threads that are blocked. */
struct list blocked_list;

/* States in a thread's life cycle. */
enum thread_status
{
    THREAD_RUNNING,     /* Running thread. */
    THREAD_READY,       /* Not running but ready to run. */
    THREAD_BLOCKED,     /* Waiting for an event to trigger. */
    THREAD_DYING        /* About to be destroyed. */
};

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

/* A kernel thread or user process.

   Each thread structure is stored in its own 4 kB page.  The
   thread structure itself sits at the very bottom of the page
   (at offset 0).  The rest of the page is reserved for the
   thread's kernel stack, which grows downward from the top of
   the page (at offset 4 kB).  Here's an illustration:

        4 kB +---------------------------------+
             |          kernel stack           |
             |                |                |
             |                |                |
             |                V                |
             |         grows downward          |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             +---------------------------------+
             |              magic              |
             |                :                |
             |                :                |
             |               name              |
             |              status             |
        0 kB +---------------------------------+

   The upshot of this is twofold:

      1. First, `struct thread' must not be allowed to grow too
         big.  If it does, then there will not be enough room for
         the kernel stack.  Our base `struct thread' is only a
         few bytes in size.  It probably should stay well under 1
         kB.

      2. Second, kernel stacks must not be allowed to grow too
         large.  If a stack overflows, it will corrupt the thread
         state.  Thus, kernel functions should not allocate large
         structures or arrays as non-static local variables.  Use
         dynamic allocation with malloc() or palloc_get_page()
         instead.

   The first symptom of either of these problems will probably be
   an assertion failure in thread_current(), which checks that
   the `magic' member of the running thread's `struct thread' is
   set to THREAD_MAGIC.  Stack overflow will normally change this
   value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
   the run queue (thread.c), or it can be an element in a
   semaphore wait list (synch.c).  It can be used these two ways
   only because they are mutually exclusive: only a thread in the
   ready state is on the run queue, whereas only a thread in the
   blocked state is on a semaphore wait list. */


struct thread
{
    /* Owned by thread.c. */
    tid_t tid;                          /* Thread identifier. */
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */
    int priority;                       /* Priority. */
    int64_t blocked_ticks;              /* If the thread is blocked, it will
                                         * be unblock after blocked ticks. */
    struct list_elem allelem;           /* List element for all threads list. */

    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /* List element. */

    // for BSD:
    int nice;
    fp recent_cpu;



#ifdef USERPROG
    /* Owned by userprog/process.c. */
    uint32_t *pagedir;              /* Page directory. */

  struct list locks;                /*lock list to put all locks for processes*/
  struct thread *parent;            /* the thread representing the parent
                                     * process(if any) of the current process*/

  struct list child_list;           /* list for all children of this thread*/
  struct list file_fd_list;         /* list for files of this process and
                                     * their corresponding information*/

  struct semaphore child_entry_sema;/* a semaphore to secure synchronization
                                     * when a child is added to the process's
                                     * children list*/

  struct semaphore child_load_sema; /* a semaphore to secure synchronization
                                     * when a child process is being loaded*/

  bool load_success;                /* a bool that a child can pass to its
                                     * parent during start_process() to decide
                                     * if we should terminate the execution
                                     * and return a -1*/

  bool in_syscall;                  /* to indicate the process is running
                                     * with a system call*/
#endif

    struct hash *spt_hash_table;

    /* Owned by thread.c. */
    unsigned magic;                     /* Detects stack overflow. */
};


/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);
size_t threads_ready(void);

void thread_tick (void);
void thread_print_stats (void);
void set_thread_blocked_ticks(struct thread *t, int64_t ticks);
void unblock_thread_with_enough_ticks(int64_t ticks);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

/* Performs some operation on thread t, given auxiliary data AUX. */
typedef void thread_action_func (struct thread *t, void *aux);
void thread_foreach (thread_action_func *, void *);

int thread_get_priority (void);
void thread_set_priority (int);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);
bool compare_priority(const struct list_elem *e1,
                      const struct list_elem *e2, void *aux);
int mlfqs_calculatePriority(struct thread *t);
struct list *get_ready_list(void);
void upDate_donate_chain(struct thread *t, int new_priority);
void update_load_avg(void);
void update_recent_cpu(void);
void update_BSD(void);
struct thread* lookup_tid(tid_t tid);

#endif /* threads/thread.h */
