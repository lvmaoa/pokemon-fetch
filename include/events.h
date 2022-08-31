#ifndef EVENTS_H_
#define EVENTS_H_

struct node 
{
    void *function;
    struct node* next;
};

struct queue
{
    struct node *front, *back;
};

void enQueueEvent(struct queue* inQ, void *inFun);

void deQueueEvent(struct queue *inQ);

void *executeEvent(struct queue *inQ);

void printQueue(struct queue *inQ);

#endif

