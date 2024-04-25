#ifndef QUEUEH
#define QUEUEH
#include "lwp.h"

typedef struct tNode{
    thread theThread;
    struct tNode *next;
} threadNode;

void enqueue(threadNode *head, threadNode *tail, thread newThread);
thread dequeue(threadNode *head, threadNode *tail);
void freeQueue(threadNode *head, threadNode *tail);

#endif