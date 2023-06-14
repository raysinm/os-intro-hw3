#include <stdbool.h>
#include <stdio.h>

typedef enum{
    QUEUE_SUCCESS,
    QUEUE_EMPTY,
    QUEUE_FULL,
    QUEUE_NOT_FOUND
} QueueError;

typedef struct QueueNode QueueNode;
typedef struct RequestQueue RequestQueue;

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

QueueNode* QueueNode_create(int fd);

RequestQueue* RequestQueue_create(int capacity);
void RequestQueue_destroy(RequestQueue *queue);
bool RequestQueue_isempty(RequestQueue* queue);
int RequestQueue_dequeue(RequestQueue* queue, QueueError* error);
QueueError RequestQueue_queue(RequestQueue* queue, int new_fd);
QueueError RequestQueue_dequeue_item(RequestQueue* queue, int target_fd);
int RequestQueue_front(RequestQueue* queue, QueueError* error);
int RequestQueue_size(RequestQueue* queue);