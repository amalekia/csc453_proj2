#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <sys/mman.h>
#include "lwp.h"
#include "rr.h"

int thread_count = 0;

thread current_thread = NULL;

struct scheduler rr_publish = {init_rr, shutdown_rr, admit_rr, remove_rr, next_rr, qlen_rr};
scheduler CurrentScheduler = &rr_publish;

extern void lwp_exit(int exitval) {
    //terminates calling thread and switches to another thread if any

    unsigned int exit_status = MKTERMSTAT(LWP_TERM, exitval);
    current_thread->status = exit_status;

    // yield will reassign current_thread
    // and advance the scheduler to the next thread
    lwp_yield();
}

static void lwp_wrap(lwpfun fun, void *arg) {
    //call the function and pass the argument
    //when the function returns, call lwp_exit()
    int rval;
    rval = fun(arg);
    lwp_exit(rval);
}

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

    //saves location of stack allocation for munmap()
    void* stack_alloc = mmap(NULL, rlim.rlim_cur, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);

    if (stack_alloc == MAP_FAILED || rlim.rlim_cur == RLIM_INFINITY) {
        return NO_THREAD;
    }

    void* stack = (void*)((char*)stack_alloc + rlim.rlim_cur);

    //defines the context for the new thread and sets the state to the initial values
    struct threadinfo_st* new_thread = (struct threadinfo_st*)malloc(sizeof(struct threadinfo_st));
    new_thread->tid = tid;
    new_thread->stack = stack;
    new_thread->stacksize = rlim.rlim_cur;
    new_thread->state.rdi = (unsigned long)argument; //argument
    new_thread->state.rsi = (unsigned long)function; //function   
    new_thread->state.fxsave=FPU_INIT;

    // push return address onto stack
    *(unsigned long*)(stack - sizeof(unsigned long)) = (unsigned long)lwp_wrap;
    //lwp wrap should be the return address

    //assign these when you know all locals and stuff is put on stack
    new_thread->state.rsp = (unsigned long)stack - sizeof(unsigned long); //stack pointer
    new_thread->state.rbp = (unsigned long)stack; //base pointer
    // push base pointer onto stack

    //call lwp_wrap() to make funciton call and cleanup but put lwp_wrap where return address is so that it will trick program and run that

    //admit the new thread to the scheduler
    CurrentScheduler->admit(new_thread);

    //increment thread count and assign thread id
    thread_count++;
    tid = thread_count;

    //after calling the process and stack is popped, free mem allocated for stack
    munmap(stack_alloc, rlim.rlim_cur);

    return tid;
}

extern void lwp_start(void) {
    //converts the calling thread into a LWP and lwp_yield()s to whichever 
    //thread is selected by the scheduler

    thread start_thread = (thread)malloc(sizeof(context));
    start_thread->tid = 0;
    start_thread->lib_one = NULL;
    start_thread->lib_two = NULL;
    start_thread->status = LWP_LIVE;
    CurrentScheduler->admit(start_thread);
    current_thread = start_thread;
    lwp_yield();
}

extern void lwp_yield(void) {
    //uses swap_rfiles to load its content

    thread prev_thread = current_thread;
    if (LWPTERMINATED(prev_thread->status)) {
        CurrentScheduler->remove(prev_thread);
    }

    current_thread = CurrentScheduler->next();
    
    // there is no next thread
    if (current_thread == NULL) {
        lwp_exit(prev_thread->status); 
    }

    // the last thread is the original thread - good to exit
    if (prev_thread == current_thread) {
        return;
    }
    swap_rfiles(&prev_thread->state, &current_thread->state);
}

extern tid_t lwp_gettid(void) {
    //return thread id of the calling LWP
    if (current_thread == NULL) { 
        return 0; 
    }
    else { 
        return current_thread->tid; 
    }
}

extern tid_t lwp_wait(int *status) {
    //waits for the thread with the given id to terminate
}

extern void lwp_set_scheduler(scheduler new_scheduler) {
    //installs a new scheduling function
    if (new_scheduler == NULL) {
        return;
    }
    CurrentScheduler = new_scheduler;

    // confirm that current scheduler has non NULL init func before calling
    if (CurrentScheduler->init) {
        CurrentScheduler->init();
    }
}

extern scheduler lwp_get_scheduler(void) {
    //find out what the current scheduler is
    return CurrentScheduler;
}

extern thread tid2thread(tid_t tid) {
    //maps a thread id to a context
}
