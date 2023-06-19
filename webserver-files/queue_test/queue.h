#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>

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
    struct timeval arrival;
    QueueNode* next;
};

struct RequestQueue{
    QueueNode* head;
    QueueNode* tail;
    int capacity;
    int size;
};

QueueNode* QueueNode_create(int fd, struct timeval arrival);

RequestQueue* RequestQueue_create(int capacity);
void RequestQueue_destroy(RequestQueue *queue);
bool RequestQueue_isempty(RequestQueue* queue);
struct timeval RequestQueue_head_arrival(RequestQueue* queue, QueueError* error);
int RequestQueue_dequeue(RequestQueue* queue, QueueError* error);
QueueError RequestQueue_queue(RequestQueue* queue, int new_fd, struct timeval arrival);
QueueError RequestQueue_dequeue_item(RequestQueue* queue, int target_fd);
int RequestQueue_front(RequestQueue* queue, QueueError* error);
int RequestQueue_size(RequestQueue* queue);
int* RequestQueue_get_vals(RequestQueue* queue);
void RequestQueue_drop_half_random(RequestQueue* queue);
