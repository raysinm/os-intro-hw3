#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "queue.h"



QueueNode* QueueNode_create(int fd, struct timeval arrival){
    QueueNode* node = (QueueNode*)malloc(sizeof(*node));
    node -> fd = fd;
    node-> arrival = arrival;
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

void RequestQueue_destroy(RequestQueue *queue){
    if (queue->size == 0){
        free(queue);
        return;
    }

    QueueNode* node = queue->head;
    QueueNode* next = queue->head; 
    while(node!=NULL){
        next = node->next;
        free(node);
        node = next;
    }
    free(queue);
    return;
}

bool RequestQueue_isempty(RequestQueue* queue){
    return queue->size == 0;
}


QueueError RequestQueue_queue(RequestQueue* queue, int new_fd, struct timeval arrival){
    if (queue->size == queue->capacity){
        return QUEUE_FULL;
    }

    // QueueNode* tail = queue->tail;
    QueueNode* new_node = QueueNode_create(new_fd, arrival);
    if (queue->size == 0){
        queue->tail = new_node;
        queue->head = new_node;
    }
    else{
        if(queue->tail){
            queue->tail->next = new_node;
        }
        queue->tail = new_node;
    }

    ++(queue->size);
    return QUEUE_SUCCESS;
}

struct timeval RequestQueue_head_arrival(RequestQueue* queue, QueueError* error){
    if (queue->size == 0){
        *error = QUEUE_EMPTY;
        return (struct timeval){0,0};
    }
    return queue->head->arrival;
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
        queue->head = NULL;
        queue->tail = NULL;
    }

    *error = QUEUE_SUCCESS;

    return ret_fd;
}

QueueError RequestQueue_dequeue_item(RequestQueue* queue, int target_fd){
    if(queue->size == 0 || queue->head==NULL){
        return QUEUE_EMPTY;
    }

    if (queue->head->fd == target_fd){

        QueueNode* next_temp = queue->head->next;
        free(queue->head);
        queue->head = next_temp;

        --(queue->size);
        if (queue->size == 0){
            queue->head = NULL;
            queue->tail = NULL;
        }
        // else if (queue->size == 1){
        //     queue->tail = queue->head;
        // }
        return QUEUE_SUCCESS;
    }

    QueueNode* node = queue->head->next;
    QueueNode* prev = queue->head;
    
    while(node != NULL){
        if (node->fd == target_fd){
            prev->next = node->next;

            if(queue->head==node){
                queue->head = node->next;
            }

            if (queue->tail==node){    //It's the last node
                queue->tail = prev;
            }
            
            free(node);

            --(queue->size);
            if (queue->size == 0){
                queue->head = NULL;
                queue->tail = NULL;
            }

            return QUEUE_SUCCESS;
        }
        prev = node;
        node = node->next;
    }

    return QUEUE_NOT_FOUND;


}



int RequestQueue_front(RequestQueue* queue, QueueError* error){
    if (queue->size!=0 && queue->head){
        *error = QUEUE_SUCCESS;
        return queue->head->fd;
    }

    *error = QUEUE_EMPTY;
    return -1;
}

int RequestQueue_size(RequestQueue* queue){
    return queue->size;
}

int* RequestQueue_get_vals(RequestQueue* queue){
    if (queue->size == 0){
        return NULL;
    }
    int* vals = (int*)malloc(sizeof(*vals)*queue->size);

    QueueNode* node = queue->head;
    
    int i=0;
    while(node!=NULL){
        vals[i] = node->fd;
        node = node->next;
        ++i;
    }
    return vals;
}

void RequestQueue_drop_half_random(RequestQueue* queue){
    if (RequestQueue_isempty(queue)){
        return;
    }
    int end_num = (int)((queue->size)/2);
    while(queue->size > end_num){
        int* vals = RequestQueue_get_vals(queue);
        if (vals != NULL){
            int i_drop = rand()%(queue->size + 1);
            int fd_drop = vals[i_drop];
            RequestQueue_dequeue_item(queue, fd_drop);
            free(vals);
        }
    }
    return;
}
