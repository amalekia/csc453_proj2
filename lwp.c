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

thread current_thread;
thread terminated_queue;
thread waiting_queue;

struct scheduler rr_publish = {init_rr, shutdown_rr, admit_rr, remove_rr, next_rr, qlen_rr};
scheduler CurrentScheduler = &rr_publish;

void add_terminated(thread newThread) {
    if (terminated_queue == NULL) {
        terminated_queue = newThread;
        terminated_queue->lib_one = NULL;
    }
    else {
        thread current = terminated_queue;
        while (current->lib_one != NULL) {
            current = current->lib_one;
        }
        newThread->lib_one = NULL;
        current->lib_one = newThread;
    }
}

void add_waiter(thread newThread) {
    if (waiting_queue == NULL) {
        waiting_queue = newThread;
        waiting_queue->lib_one = NULL;
    }
    else {
        thread current = waiting_queue;
        while (current->lib_one != NULL) {
            current = current->lib_one;
        }
        newThread->lib_one = NULL;
        current->lib_one = newThread;
    }
}

void remove_terminated(thread removing) {
    if (terminated_queue == NULL || removing == NULL) {
        return;
    }

    if (terminated_queue == removing) {
        terminated_queue = removing->lib_one;
        return;
    }

    thread current = terminated_queue;
    thread prev;
    while (current != removing && current != NULL) {
        prev = current;
        current = current->lib_one;
    }

    if (current == NULL) {
        return;
    }
    prev->lib_one = current->lib_one;
}

void remove_waiting(thread removing) {
    if (waiting_queue == NULL || removing == NULL) {
        return;
    }

    if (waiting_queue == removing) {
        waiting_queue = removing->lib_one;
        return;
    }

    thread current = waiting_queue;
    thread prev;
    while (current != removing && current != NULL) {
        prev = current;
        current = current->lib_one;
    }

    if (current == NULL) {
        return;
    }

    prev->lib_one = current->lib_one;
}

extern void lwp_exit(int exitval) {
    //terminates calling thread and switches to another thread if any

    unsigned int exit_status = MKTERMSTAT(LWP_TERM, exitval);
    current_thread->status = exit_status;

    CurrentScheduler->remove(current_thread);
    add_terminated(current_thread);

    // if something is waiting, move it back to the scheduler 
    if (waiting_queue != NULL) {
        waiting_queue->exited = terminated_queue;
        CurrentScheduler->admit(waiting_queue);
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
    // push base pointer onto stack

    //call lwp_wrap() to make funciton call and cleanup but put lwp_wrap where return address is so that it will trick program and run that

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
    start_thread->stack = NULL;
    start_thread->tid = 0;
    start_thread->lib_one = NULL;
    start_thread->lib_two = NULL;
    start_thread->status = LWP_LIVE;
    CurrentScheduler->admit(start_thread);
    current_thread = start_thread;
    waiting_queue = NULL;
    terminated_queue = NULL;
    lwp_yield();
}

extern void lwp_yield(void) {
    //uses swap_rfiles to load its content

    thread prev_thread = current_thread;
    current_thread = CurrentScheduler->next();

    if (current_thread == NULL) {
        exit(prev_thread->status);
    }

    // void swap_rfiles(rfile *old, rfile *new)
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

    if (CurrentScheduler->qlen() <= 1){
        return NO_THREAD;
    }

    if (terminated_queue == NULL) { // no terminated threads
        CurrentScheduler->remove(current_thread);
        add_waiter(current_thread);
        lwp_yield();

        thread terminated_node = current_thread->exited;
        remove_terminated(terminated_queue);
        remove_waiting(current_thread);
        if (terminated_node != NULL) {
            if (status != NULL) {
                *status = terminated_node->status;
            }
            tid_t ret_val = terminated_node->tid;
            free(terminated_node);
            return ret_val;
        }
    }
    else {
        thread firstTerminated = terminated_queue;
        remove_terminated(terminated_queue);
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
}