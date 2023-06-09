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
RequestQueue* running_q;    // Maybe should beregular list

void* request_manager(){
    QueueError error;
    while(1){
        /*START LOCK*/
        bool waiting_q_empty = RequestQueue_isempty(waiting_q); 
        /*END LOCK*/
        if (waiting_q_empty){
            //TODO: wait
        }
        else{
            /*START LOCK*/
            int connfd = RequestQueue_dequeue(waiting_q, &error);
            /*END LOCK*/
            if (error == QUEUE_SUCCESS){
                requestHandle(connfd);
                Close(connfd);
            }
        }
    }
}

// HW3: Parse the new arguments too
void getargs(int* port, int* threadsnum, int* queuesize, int argc, char *argv[])
{
    if (argc < 2) {
	fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	exit(1);
    }
    *port = atoi(argv[1]);
    *threadsnum = atoi(argv[2]);
    *queuesize = atoi(argv[3]);
    //TODO: Add parsing for more args
}


int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen, threadsnum, queuesize;
    struct sockaddr_in clientaddr;


    getargs(&port, &threadsnum, &queuesize, argc, argv);

    waiting_q = RequestQueue_create(queuesize);
    running_q = RequestQueue_create(queuesize);
    

    pthread_t ths[threadsnum];

    for (int i=0; i<threadsnum; i++){
        if(pthread_create(&ths[i], NULL, &request_manager, NULL)!=0){
            return 1;   //TODO: Error handling
        }
    }

    // 
    // HW3: Create some threads...
    //
    //TODO: Create threads in a loop?

    listenfd = Open_listenfd(port);
    while (1) {
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

	// 
	// HW3: In general, don't handle the request in the main thread.
	// Save the relevant info in a buffer and have one of the worker threads 
	// do the work. 
	// 

    }

}


    


 
