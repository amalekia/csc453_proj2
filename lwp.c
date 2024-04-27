#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <sys/mman.h>
#include "lwp.h"
#include "rr.h"
#include "queue.h"

int thread_count = 0;

threadNode *terminatedHead;
threadNode *terminatedTail;
threadNode *waitingHead;
threadNode *waitingTail;

thread current_thread;

struct scheduler rr_publish = {init_rr, shutdown_rr, admit_rr, remove_rr, next_rr, qlen_rr};
scheduler CurrentScheduler = &rr_publish;

extern void lwp_exit(int exitval) {
    //terminates calling thread and switches to another thread if any

    unsigned int exit_status = MKTERMSTAT(LWP_TERM, exitval);
    current_thread->status = exit_status;

    if (LWPTERMINATED(current_thread->status)) {
        CurrentScheduler->remove(current_thread);
        enqueue(terminatedHead, terminatedTail, current_thread); // add to terminated queue
    } else {
        CurrentScheduler->remove(current_thread);
        enqueue(waitingHead, waitingTail, current_thread);
    }

    // if something is waiting, move back to scheduler 
    if (waitingHead != NULL) {
        thread waiter = dequeue(waitingHead, waitingTail);
        CurrentScheduler->admit(waiter);
    }

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
        rlim.rlim_cur = (8*1024*1024); // 8MB
    }
    else if ((remainder = pagesize % rlim.rlim_cur) != 0) {
        rlim.rlim_cur += remainder;
    }

    //saves location of stack allocation for munmap()
    unsigned long* stack_alloc = mmap(NULL, rlim.rlim_cur, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);

    if (stack_alloc == MAP_FAILED || rlim.rlim_cur == RLIM_INFINITY) {
        return NO_THREAD;
    }

    unsigned long* stack = (unsigned long*)((char*)stack_alloc + rlim.rlim_cur);

    //defines the context for the new thread and sets the state to the initial values
    struct threadinfo_st* new_thread = (struct threadinfo_st*)malloc(sizeof(struct threadinfo_st));
    //increment thread count and assign thread id
    thread_count++;
    tid = thread_count;
    new_thread->state.fxsave=FPU_INIT;
    new_thread->tid = tid;
    new_thread->stack = stack;
    new_thread->stacksize = rlim.rlim_cur;
    new_thread->state.rdi = (unsigned long)function; //function
    new_thread->state.rsi = (unsigned long)argument; //argument   

    // push return address onto stack
    *(stack - 2) = (unsigned long)lwp_wrap;
    //lwp wrap should be the return address

    //assign these when you know all locals and stuff is put on stack
    new_thread->state.rsp = (unsigned long)(stack - 3); //stack pointer
    new_thread->state.rbp = (unsigned long)(stack - 3); //base pointer

    //admit the new thread to the scheduler
    CurrentScheduler->admit(new_thread);

    //after calling the process and stack is popped, free mem allocated for stack
    // munmap(stack_alloc, rlim.rlim_cur); - done later in exit

    return tid;
}

extern void lwp_start(void) {
    //converts the calling thread into a LWP and lwp_yield()s to whichever 
    //thread is selected by the scheduler

    thread start_thread = (thread)malloc(sizeof(context));
    start_thread->stack = NULL; //?
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
    current_thread = CurrentScheduler->next();

    if (current_thread == NULL) {
        exit(prev_thread->status);
    }

    // the last thread is the original thread - good to exit
    if (prev_thread == current_thread) {
        return;
    }

    // void swap_rfiles(rfile *old, rfile *new)
    swap_rfiles(&prev_thread->state, &current_thread->state); // crash here
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
    //all memory freeing happens here - every thread must be waited on

    if (terminatedHead == NULL) { // no terminated threads
        CurrentScheduler->remove(current_thread);
        enqueue(waitingHead, waitingTail, current_thread);
    }
    else {
        thread firstTerminated = dequeue(terminatedHead, terminatedTail);
        if (status != NULL) {
            *status = firstTerminated->status;
        }
        tid_t ret_val = firstTerminated->tid;
        free(firstTerminated);
        return ret_val;
    }
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
    threadNode *temp = terminatedHead;
    while (temp != NULL) {
        if (temp->theThread->tid == tid) {
            return temp->theThread;
        }
        temp = temp->next;
    }
    temp = waitingHead;
    while (temp != NULL) {
        if (temp->theThread->tid == tid) {
            return temp->theThread;
        }
        temp = temp->next;
    }
    return NULL;
}
