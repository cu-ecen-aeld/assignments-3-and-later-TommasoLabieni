#ifndef LINKED_LIST_LIST_H
#define LINKED_LIST_LIST_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>

typedef struct
{
    pthread_t *tid;
    int cfd;
    bool complete;
    bool joined;
} ThreadData;

typedef struct Node
{
    ThreadData *data;
    struct Node *next;
} ListNode;

typedef ListNode *List;

void AppendNode(List *l, ListNode *node);
ListNode *CreateNode(ThreadData *data);
void FreeList(List *l);
void PrintList(List l);

#endif /* LINKED_LIST_LIST_H */