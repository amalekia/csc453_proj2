#include <stdio.h>
#include "lwp.h"
// thread head = NULL;

void init_rr() {

}

void shutdown_rr() {

}

void admit_rr(thread t){

}

void remove_rr(thread t){

}

thread next_rr() {
    return NULL;
}

int qlen_rr() {
    return 0;
}

struct scheduler rr_publish = {NULL, NULL, admit_rr, remove_rr, next_rr, qlen_rr};
scheduler RoundRobin = &rr_publish;
// Calling:
// thread nxt;
// nxt = RoundRobin->next()