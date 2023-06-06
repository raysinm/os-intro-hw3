#include <stdio.h>

#include "queue.h"

struct QueueNode{
    int fd;
    QueueNode* next;
};

struct RequestQueue{
    QueueNode* head;
    QueueNode* tail;
    int capacity;
    int size;
};

QueueNode* QueueNode_create(int fd){
    struct QueueNode* node = (QueueNode*)malloc(sizeof(node));
    node -> fd = fd;
    node -> next = NULL;
    return node;
}


RequestQueue* RequestQueue_create(int capacity){
    struct RequestQueue* queue = (RequestQueue*)malloc(sizeof(queue));
    queue -> capacity = capacity;
    queue -> head = NULL;
    queue -> tail = NULL;
    queue -> size = 0;
    return queue;
}

bool RequestQueue_isempty(struct RequestQueue* queue){
    return queue->size == 0;
}

int RequestQueue_front(struct RequestQueue* queue){
    if (RequestQueue_isempty(queue)){
        return -1;
    }
    struct QueueNode* head = queue->head;
    return head->fd;
}


//TODO: Change return type to enum to deal with errors in main
int RequestQueue_queue(struct RequestQueue* queue, int new_fd){
    struct QueueNode* tail = queue->tail;
    QueueNode* new_node = QueueNode_create(new_fd);
    //TODO: Finish
}