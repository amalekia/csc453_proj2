#include <stdio.h>
#include <stdlib.h>
#include "lwp.h"

typedef struct queueNode{
    int index;
    thread theThread;
    struct queueNode *next;
} node;

node *head;
node *tail;
int qlen; // qlen of zero means empty, qlen 1 being just the head, so head = tail, and so on

void init_rr(void) {
    head = NULL;
    tail = NULL;
    qlen = 0;
}

void shutdown_rr(void) {
    node *current = head;
    while (current != NULL)
    {
        node *target = current;
        current = current->next;
        free(target);
    }
    head = NULL;
    tail = NULL;
    qlen = 0;
    return;
}

// Done for now, needs testing
void admit_rr(thread newThread){
    if (newThread == NULL)
    {
        return;
    }
    if (head == NULL)
    {
        head = (node *)malloc(sizeof(node));
        head->theThread = newThread;
        head->next = NULL;
        head->index = qlen++;
        tail = head;
    }
    else
    {
        tail->next = (node *)malloc(sizeof(node));
        tail->next->theThread = newThread;
        tail->next->index = qlen++;
        tail->next->next = NULL;
        tail = tail->next;
    }
}

void remove_rr(thread removing){
    if (removing == NULL)
    {
        return;
    }

    node *current = head;
    while (current != NULL)
    {
        if (current->next->theThread == removing)
        {
            node *removalTarget = current->next;
            current->next = current->next->next;
            free(removalTarget);
            qlen--;
            return;
        }
        current = current->next;
    }
    return;
}

thread next_rr(void) {
    if (head == NULL || qlen == 0)
    {
        return NULL;
    }
    thread nextThread = head->theThread;
    head = head->next;
    return nextThread;
}

int qlen_rr(void) {
    return qlen;
}

// -------------------------------------
// --------- Debug Code Below ----------
// -------------------------------------

void printSchedule() {
    if (head == NULL)
    {
        printf("Schedule Empty\n");
        return;
    }
    node *current = head;
    while (current != NULL)
    {
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