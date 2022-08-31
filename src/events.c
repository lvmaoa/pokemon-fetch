#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "../include/events.h"

struct node *newnode(void *inFun)
{
    struct node *temp = (struct node*) malloc(sizeof(struct node));
    temp->function = inFun;
    temp->next = NULL;
    return temp;
}

void enQueueEvent(struct queue* inQ, void *inFun)
{
    struct node *temp = newnode(inFun);
    
    if(inQ->back == NULL)
    {
        inQ->front = temp;
        inQ->back = temp;
        return;
    }

    inQ->back->next = temp;
    inQ->back = temp;
}

// TODO: Fix deQueueEvent -> not dequeuing
void deQueueEvent(struct queue *inQ)
{
    if (inQ->front == NULL)
        return;

    struct node *temp = inQ->front;
    inQ->front = inQ->front->next;

    if (inQ->front == NULL)
        inQ->back = NULL;

    free(temp);
}

void *executeEvent(struct queue *inQ)
{
    void *ret;
    if (inQ->front != NULL)
    {
        void* (*fun)() = inQ->front->function;
        ret = fun();
        deQueueEvent(inQ);
    }
    return ret;
}

void printQueue(struct queue *inQ)
{
    struct node *temp;
    temp = inQ->front;
    while(temp->next != NULL)
    {
        printf("%p", temp->function);
    }
}

