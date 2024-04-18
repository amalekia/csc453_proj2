#include stdio.h
#include lwp.h
#include rr.h


scheduler s = &rr;

lwp_create(lwpfun function, void *argument) {
    //most of the work will be done here.
    //creates a new thread and sets up its context 
    //so that when it is selected by the scheduler to run
}

lwp_start(void) {
    //converts the calling thread into a LWP and lwp_yield()s to whichever 
    //thread is selected by the scheduler
}

lwp_yield(void) {
    //uses swap_rfiles to load its content
}

lwp_exit(int exitval) {
    //terminates calling thread and switches to another thread if any
}

lwp_gettid(void) {
    //return thread id of the calling LWP
}

lwp_wait(int *status) {
    //waits for the thread with the given id to terminate
}

lwp_set_scheduler(scheduler sched) {
    //installs a new scheduling function
}

lwp_get_scheduler(void) {
    //find out what the current scheduler is
}

thread tid2thread(tid_t tid) {
    //maps a thread id to a context
}
