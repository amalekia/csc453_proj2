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