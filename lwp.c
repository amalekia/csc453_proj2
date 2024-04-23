#include <stdio.h>
#include <stdlib.h>
#include "lwp.h"
#include "rr.h"


// scheduler s = &rr;
thread current_thread = NULL;

struct scheduler rr_publish = {init_rr, shutdown_rr, admit_rr, remove_rr, next_rr, qlen_rr};
scheduler CurrentScheduler = &rr_publish;

extern tid_t lwp_create(lwpfun function, void *argument) {
    //most of the work will be done here.
    //creates a new thread and sets up its context 
    //so that when it is selected by the scheduler to run

    tid_t tid;
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
}

extern void lwp_exit(int exitval) {
    //terminates calling thread and switches to another thread if any
    lwp_yield();
}

extern tid_t lwp_gettid(void) {
    //return thread id of the calling LWP
    if (current_thread == NULL){ return 0; }
    else { return current_thread->tid; }
}

extern tid_t lwp_wait(int *status) {
    //waits for the thread with the given id to terminate
}

extern void lwp_set_scheduler(scheduler new_scheduler) {
    //installs a new scheduling function
    if (new_scheduler == NULL)
    {
        return;
    }
    CurrentScheduler = new_scheduler;
    if (CurrentScheduler->init)
    {
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
