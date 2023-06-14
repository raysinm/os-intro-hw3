#include <pthread.h>

#include "segel.h"
#include "request.h"
#include "queue.h"

// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//
RequestQueue* waiting_q;
RequestQueue* running_q;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_full = PTHREAD_COND_INITIALIZER;

void* request_manager(){
    QueueError err;
    while (1){
        
        pthread_mutex_lock(&lock);
        while (RequestQueue_isempty(waiting_q)){
            printf("Thread: waiting for request\n");
        //UNLOCK - ?
            pthread_cond_wait(&cond_empty, &lock);
        }
        pthread_mutex_unlock(&lock);

        pthread_mutex_lock(&lock);
        int req_fd = RequestQueue_dequeue(waiting_q, &err);   //Also deletes the req from waiting_q
        pthread_mutex_unlock(&lock);
        printf("Thread: Handling request\n");

        if (err != QUEUE_SUCCESS){
            //HANDLE ERROR
        }
        else{
            pthread_mutex_lock(&lock);        
            RequestQueue_queue(running_q, req_fd);
            pthread_mutex_unlock(&lock);
            
            requestHandle(req_fd);
            Close(req_fd);
            printf("Thread: handled and closed the request\n");

            pthread_mutex_lock(&lock);        
            err = RequestQueue_dequeue_item(running_q, req_fd);
            //TODO: Check for errors
            pthread_mutex_unlock(&lock);
        }
    }
}

// HW3: Parse the new arguments too
void getargs(int* port, int* threadsnum, int* queuesize, char* schedalg, int argc, char *argv[])
{
    if (argc < 5) {
	fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	exit(1);
    }
    *port = atoi(argv[1]);
    *threadsnum = atoi(argv[2]);
    *queuesize = atoi(argv[3]);
    strcpy(schedalg, argv[4]);
    //TODO: Add parsing for more args
}


int main(int argc, char *argv[])
{
    // pthread_mutex_init(&lock, NULL);
    // pthread_cond_init(&cond_full, NULL);
    // pthread_cond_init(&cond_empty, NULL);

    int listenfd, connfd, port, clientlen, threadsnum, queuesize;
    char* schedalg;
    struct sockaddr_in clientaddr;
    QueueError err;

    getargs(&port, &threadsnum, &queuesize, schedalg, argc, argv);

    // Initialize queues
    waiting_q = RequestQueue_create(queuesize);
    running_q = RequestQueue_create(queuesize);
    
    // Initialize and create thread pool
    pthread_t* ths = (pthread_t*)malloc(threadsnum*sizeof(ths));

    for (int i=0; i<threadsnum; i++){
        if(pthread_create(&ths[i], NULL, &request_manager, NULL)!=0){
            return 1;   //TODO: Error handling
        }
    }

    listenfd = Open_listenfd(port);

    printf("Server: Starting to listen to incoming requests :)\n");

    while (1) {     // Server recieving requests

        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        printf("Server: Recieved a request\n");

        pthread_mutex_lock(&lock);
        while(RequestQueue_size(waiting_q) + RequestQueue_size(running_q) > queuesize){
            if (strcmp(schedalg, "block")==0){
                pthread_cond_wait(&cond_full, &lock);
            }
            else if (strcmp(schedalg, "drop_tail")==0){
                Close(connfd);
            }
            else if (strcmp(schedalg, "drop_head")==0){
                
            }
            else if (strcmp(schedalg, "block_flush")==0){
            }
            else if (strcmp(schedalg, "dynamic")==0){
            }
            else if (strcmp(schedalg, "drop_random")==0){
            }
        }
        pthread_mutex_unlock(&lock);


        //Check if all queues are full

        //Else:

        pthread_mutex_lock(&lock);        
        err = RequestQueue_queue(waiting_q, connfd);
        pthread_mutex_unlock(&lock);
        
        if ( err!= QUEUE_SUCCESS ){
            //TODO: handle error

        }
        pthread_cond_signal(&cond_empty);
        printf("Server: Forwarded request to threads\n");
    }

    RequestQueue_destroy(waiting_q);
    RequestQueue_destroy(running_q);
    
    free(ths);
    
	// HW3: In general, don't handle the request in the main thread.
	// Save the relevant info in a buffer and have one of the worker threads 
	// do the work. 
	// 

    

}


    


 
