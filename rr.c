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
int qlen;

void init_rr() {
    head = NULL;
    tail = NULL;
    qlen = 0;
}

// Need to free node by node
void shutdown_rr() {
    head = NULL;
    tail = NULL;
    qlen = 0;
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
    while (current->next != NULL)
    {
        if (current->next->theThread == removing)
        {
            node *removalTarget = current->next;
            current->next = current->next->next;
            free(removalTarget);
            return;
        }
        current = current->next;
    }
    return;
}

thread next_rr() {
    return NULL;
}

int qlen_rr() {
    return qlen;
}

struct scheduler rr_publish = {NULL, NULL, admit_rr, remove_rr, next_rr, qlen_rr};
scheduler RoundRobin = &rr_publish;
// Calling:
// thread nxt;
// nxt = RoundRobin->next()

// -------------------------------------
// --------- Debug Code Below ----------
// -------------------------------------

void printSchedule(){
    if (head == NULL)
    {
        printf("Schedule Empty\n");
        return;
    }
    node *current = head;
    while (current->next != NULL)
    {
        printf("Thread index: %d, threadId: %ld\n", current->index, current->theThread->tid);
        current = current->next;
    }
    printf("qlen: %d\n", qlen);
    return;
}

// gcc -Wall -std=gnu99 -pedantic -o rr rr.o
// gcc -Wall -std=gnu99 -pedantic -c rr.c
int main(int argc, char *argv[])
{
    init_rr();
    printSchedule();
    return 0;
}