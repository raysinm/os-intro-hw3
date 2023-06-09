#include <stdio.h>
#include <stdlib.h>

#include "queue.h"



QueueNode* QueueNode_create(int fd){
    QueueNode* node = (QueueNode*)malloc(sizeof(node));
    node -> fd = fd;
    node -> next = NULL;
    return node;
}


RequestQueue* RequestQueue_create(int capacity){
    RequestQueue* queue = (RequestQueue*)malloc(sizeof(queue));
    queue -> capacity = capacity;
    queue -> head = NULL;
    queue -> tail = NULL;
    queue -> size = 0;
    return queue;
}

bool RequestQueue_isempty(RequestQueue* queue){
    return queue->size == 0;
}


QueueError RequestQueue_queue(RequestQueue* queue, int new_fd){
    if (queue->size == queue->capacity){
        return QUEUE_FULL;
    }

    QueueNode* tail = queue->tail;
    QueueNode* new_node = QueueNode_create(new_fd);
    if (tail == NULL){
        queue->tail = new_node;
        queue->head = new_node;
        ++(queue->size);
        return QUEUE_SUCCESS;
    }

    

    tail->next = new_node;
    queue->tail = tail->next;
    ++(queue->size);
    return QUEUE_SUCCESS;
}

int RequestQueue_dequeue(RequestQueue* queue, QueueError* error){
    if (queue->size == 0){
        *error = QUEUE_EMPTY;
        return -1;
    }

    QueueNode* head = queue->head;
    QueueNode* next_head = head->next;
    
    int ret_fd = head->fd;
    free(head);
    queue->head = next_head;
    
    --(queue->size);
    if (queue->size == 0){
        queue->tail = NULL;
    }

    *error = QUEUE_SUCCESS;

    return ret_fd;
}

int RequestQueue_front(RequestQueue* queue, QueueError* error){
    if (queue->size!=0 && queue->head){
        *error = QUEUE_SUCCESS;
        return queue->head->fd;
    }

    *error = QUEUE_EMPTY;
    return -1;
}