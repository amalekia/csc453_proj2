#include <stdio.h>
#include <stdlib.h>
#include "lwp.h"

// typedef struct queueNode{
//     int index;
//     thread theThread;
//     struct queueNode *next;
// } node;

// node *head;
// node *tail;
// int qlen; // qlen of zero means empty, qlen 1 being just the head, so head = tail, and so on

thread head;
thread next;
int qlen;

void init_rr(void) {
    head = NULL;
    next = NULL;
    qlen = 0;
}

void shutdown_rr(void) {
    thread tmp;
    thread current = head;
    while (current->sched_one != NULL) {
        tmp = current->sched_one;
        free(current);
        current = tmp;
    }
    head = NULL;
    qlen = 0;
    return;
}

void admit_rr(thread newThread) {
    if (head == NULL) {
        head = newThread;
        head->sched_one = NULL;
        head->sched_two = NULL;
    } else {
        thread current = head;
        while (current->sched_one != NULL) {
            current = current->sched_one;
        }
        newThread->sched_one = NULL;      // new->next = NULL
        newThread->sched_two = current;   // new->prev = current
        current->sched_one = newThread;   // old_tail->next = new
    }
    qlen++;
}

void remove_rr(thread removing) {
    if (removing == NULL || head == NULL) {
        return;
    }

    node *current = head;
    while (current != NULL) {
        if (current->next->theThread == removing) {
            node *removalTarget = current->next;
            current->next = current->next->next;
            free(removalTarget);
            qlen--;
            return;
        }
        current = current->next;
    }
    qlen--;
    return;
}

thread next_rr(void) {
    if (next == NULL) {
        next = head;
    } else {
        next = next->sched_one;
        if (next == NULL) {
            next = head;
        }
    }
    return next;
}

int qlen_rr(void) {
    return qlen;
}

// -------------------------------------
// --------- Debug Code Below ----------
// -------------------------------------

void printSchedule() {
    if (head == NULL) {
        printf("Schedule Empty\n");
        return;
    }
    node *current = head;
    while (current != NULL) {
        printf("Thread index: %d, tid: %ld\n", current->index, current->theThread->tid);
        current = current->next;
    }
    printf("qlen: %d\n", qlen);
    return;
}

// --- for schedule testing purposes ONLY ---
// thread createTestThread() {
//     context *thread_context;
//     thread_context = (context *)malloc(sizeof(context));
//     thread_context->tid = 1234;
//     thread testThread = thread_context;
//     return testThread;
// }

// gcc -Wall -std=gnu99 -pedantic -o rr rr.o
// gcc -Wall -std=gnu99 -pedantic -c rr.c
// valgrind --leak-check=yes ./rr
// int main(int argc, char *argv[])
// {
//     init_rr();
//     thread testThread = createTestThread();

//     admit_rr(testThread);
//     printSchedule(); // qlen = 1

//     admit_rr(testThread);
//     printSchedule(); // qlen = 2

//     remove_rr(testThread);
//     printSchedule(); // qlen = 1

//     shutdown_rr();
//     free(testThread);
//     printSchedule(); // Schedule Empty, qlen = 0

//     return 0;
// }