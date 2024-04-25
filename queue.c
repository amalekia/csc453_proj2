#include <stdio.h>
#include <stdlib.h>
#include "lwp.h"
#include "queue.h"

void enqueue(threadNode *head, threadNode *tail, thread newThread) {
    if (newThread == NULL) {
        return;
    }
    if (head == NULL) {
        head = (threadNode *)malloc(sizeof(threadNode));
        head->theThread = newThread;
        head->next = NULL;
        tail = head;
    }
    else {
        tail->next = (threadNode *)malloc(sizeof(threadNode));
        tail->next->theThread = newThread;
        tail->next->next = NULL;
        tail = tail->next;
    }
}

thread dequeue(threadNode *head, threadNode *tail) {
    if (head == NULL) {
        return 0;
    }
    thread nextThread = head->theThread;
    head = head->next;
    return nextThread;
}

void freeQueue(threadNode *head, threadNode *tail) {
    threadNode *current;
    while (head != NULL) {
        current = head;
        head = head->next;
        free(current);
    }
}