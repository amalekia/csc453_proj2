#include stdio.h
#include lwp.h
#include rr.h


scheduler s = &rr;

lwp_create() {
    //most of the work will be done here.
    //creates a new thread and sets up its context 
    //so that when it is selected by the scheduler to run
}

lwp_start() {

}

lwp_yield() {
    //uses swap_rfiles to load its content
}

lwp_exit() {
    //terminates calling thread and switches to another thread if any
}

lwp_gettid(void) {
    //return thread id of the calling LWP
}

lwp_wait(int *status) {
    //waits for the thread with the given id to terminate
}

lwp_set_scheduler(scheduler fun) {
    //installs a new scheduling function
}

lwp_get_scheduler(void) {
    //find out what the current scheduler is
}

thread tid2thread(tid_t tid) {
    //maps a thread id to a context
}
