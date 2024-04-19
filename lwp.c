#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <sys/mman.h>
#include "lwp.h"
#include "rr.h"


// scheduler s = &rr;

extern tid_t lwp_create(lwpfun function, void *argument) {
    tid_t tid;

    //allocates a stack for the new thread
    long pagesize = sysconf(_SC_PAGESIZE);

    struct rlimit rlim;
    getrlimit(RLIMIT_STACK, &rlim);

    int remainder;
    if (rlim.rlim_cur == RLIM_INFINITY) {
        rlim.rlim_cur = 8000000;
    }
    else if ((remainder = pagesize % rlim.rlim_cur) != 0) {
        rlim.rlim_cur += remainder;
    }

    void* stack = mmap(NULL, rlim.rlim_cur, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);

    if (stack == MAP_FAILED || rlim.rlim_cur == RLIM_INFINITY) {
        return NO_THREAD;
    }

    //defines the context for the new thread
    struct threadinfo_st* new_thread = (struct threadinfo_st*)malloc(sizeof(struct threadinfo_st));
    new_thread->tid = tid;
    new_thread->stack = stack;
    new_thread->stacksize = rlim.rlim_cur;
    new_thread->state.rsp = (unsigned long)stack + rlim.rlim_cur; //stack pointer
    new_thread->state.rbp = (unsigned long)stack + rlim.rlim_cur; //base pointer
    new_thread->state.rdi = (unsigned long)argument; //argument
    new_thread->state.rsi = (unsigned long)function; //function   
    new_thread->status = 0;

    return tid;
}

extern void lwp_start(void) {
    //converts the calling thread into a LWP and lwp_yield()s to whichever 
    //thread is selected by the scheduler
}

extern void lwp_yield(void) {
    //uses swap_rfiles to load its content
}

extern void lwp_exit(int exitval) {
    //terminates calling thread and switches to another thread if any
}

extern tid_t lwp_gettid(void) {
    //return thread id of the calling LWP
}

extern tid_t lwp_wait(int *status) {
    //waits for the thread with the given id to terminate
}

extern void lwp_set_scheduler(scheduler sched) {
    //installs a new scheduling function
}

extern scheduler lwp_get_scheduler(void) {
    //find out what the current scheduler is
}

extern thread tid2thread(tid_t tid) {
    //maps a thread id to a context
}
