#include <stdio.h>
#include <assert.h>

#include "queue.h"


int main(){
    fflush(stdout);
    
    int front, dequeue, ret;
    QueueError error;

    /*CREATE TEST*/
    RequestQueue* queue = RequestQueue_create(10);
    printf("CREATE TEST PASSED\n");

    /*EMPTY TEST*/
    ret = RequestQueue_front(queue, &error);
    assert(error==QUEUE_EMPTY);
    assert(ret < 0);

    ret = RequestQueue_dequeue(queue, &error);
    assert(error==QUEUE_EMPTY);
    assert(ret < 0);
    printf("EMPTY TEST PASSED\n");

    /*QUEUE TEST*/
    for(int i=0; i<10; i++){
        error = RequestQueue_queue(queue, i);
        assert(error == QUEUE_SUCCESS);
    }
    printf("QUEUE TEST PASSED\n");


    /*FULL TEST*/
    error = RequestQueue_queue(queue, 11);    // Should fail
    assert(error == QUEUE_FULL);
    printf("FULL TEST PASSED\n");


    /*FRONT TEST*/
    front = RequestQueue_front(queue, &error);
    assert(error == QUEUE_SUCCESS);
    assert(front == 0);
    front = RequestQueue_front(queue, &error);
    assert(error == QUEUE_SUCCESS);     // Should succeed twice
    assert(front == 0);     // And return 0 twice
    printf("FRONT TEST PASSED\n");
    

    /*DEQUEUE TEST*/
    dequeue = RequestQueue_dequeue(queue, &error);
    assert(error == QUEUE_SUCCESS);
    assert(dequeue == 0);
    front = RequestQueue_front(queue, &error);
    assert(error == QUEUE_SUCCESS); 
    assert(front == 1);     // Should be next in line
    
    for(int i=1; i<10; i++){
        ret = RequestQueue_dequeue(queue, &error);
        assert(error == QUEUE_SUCCESS);
        assert(ret == i);
    }

    ret = RequestQueue_dequeue(queue, &error);
    assert(error == QUEUE_EMPTY);
    assert(ret < 0);
    printf("DEQUEUE TEST PASSED\n");

    /*DEQUEUE SPECIFIC ITEM TEST*/
    error = RequestQueue_dequeue_item(queue, 0);
    assert(error == QUEUE_EMPTY);

    for (int i=0; i<4; i++){
        error = RequestQueue_queue(queue, i);
        assert(error==QUEUE_SUCCESS);
    }
    for (int i=0; i<4; i++){
        error = RequestQueue_dequeue_item(queue, i);
        fflush(stdout);

        printf("%d", i);
        assert(error == QUEUE_SUCCESS);
        assert(RequestQueue_size(queue) == 3);
        
        error = RequestQueue_queue(queue,i);
        assert(error == QUEUE_SUCCESS);
        assert(RequestQueue_size(queue) == 4);

    }
    error = RequestQueue_dequeue_item(queue, 7);
    assert(error == QUEUE_NOT_FOUND);


    printf("=====PASSED ALL TESTS=====\n");

    return 0;
}