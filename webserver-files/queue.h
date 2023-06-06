#include <stdbool.h>

//TODO: Enum

typedef struct QueueNode *QueueNode;
typedef struct RequestQueue *RequestQueue;

void initRequestQueue(struct RequeustQueue* queue, int capacity);
bool RequestQueue_isempty(RequestQueue* queue);