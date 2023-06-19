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

__thread ThreadStats th_stats;

// __thread int th_stat_count = 0;
// __thread int th_dyn_count = 0;
// // __thread int th_tot_count = 0;
// // __thread time_t arrival_time;
// // __thread time_t dispatch_time;
// __thread struct timeval arrival;
// __thread struct timeval handle;


void* request_manager(void* id){    //id is int
    QueueError err;
    
    th_stats.th_id = *((int*)id);
    th_stats.th_stat_count = 0;
    th_stats.th_dyn_count = 0;

    while (1){
        
        pthread_mutex_lock(&lock);
        while (RequestQueue_isempty(waiting_q)){
            // printf("Thread: waiting for request\n");
        //UNLOCK - ?
            pthread_cond_wait(&cond_empty, &lock);
        }
        pthread_mutex_unlock(&lock);

        pthread_mutex_lock(&lock);
        //STATS: arrival time
        th_stats.arrival = RequestQueue_head_arrival(waiting_q, &err);
        int req_fd = RequestQueue_dequeue(waiting_q, &err);   //Also deletes the req from waiting_q
        pthread_mutex_unlock(&lock);
        
        //STATS: handle time
        gettimeofday(&(th_stats.handle), NULL);
        // printf("Thread: Handling request\n");

        if (err != QUEUE_SUCCESS){
            //HANDLE ERROR
        }
        else{
            pthread_mutex_lock(&lock);        
            RequestQueue_queue(running_q, req_fd, th_stats.arrival);
            pthread_mutex_unlock(&lock);
            
            requestHandle(req_fd, &th_stats);
            Close(req_fd);
            //printf("Thread: handled and closed the request\n");

            pthread_mutex_lock(&lock);        
            err = RequestQueue_dequeue_item(running_q, req_fd);
            //TODO: Check for errors
            pthread_mutex_unlock(&lock);
        }
    }
}

void getargs(int* port, int* threadsnum, int* queuesize, char* schedalg, int* max_size, int argc, char *argv[])
{
    if (argc < 5) {
	fprintf(stderr, "Usage: %s <portnum> <threads> <queue_size> <schedalg> <max_size(optional)>\n", argv[0]);
	exit(1);
    }
    *port = atoi(argv[1]);
    *threadsnum = atoi(argv[2]);
    *queuesize = atoi(argv[3]);
    strcpy(schedalg, argv[4]);
    if (strcpy(schedalg, "dynamic") == 0 && argc==6){
        *max_size = atoi(argv[5]);
    }
}


int main(int argc, char *argv[])
{
    // pthread_mutex_init(&lock, NULL);
    // pthread_cond_init(&cond_full, NULL);
    // pthread_cond_init(&cond_empty, NULL);

    int listenfd, connfd, port, clientlen, threadsnum, queuesize, max_size;
    char schedalg[11];
    struct sockaddr_in clientaddr;
    QueueError err;

    getargs(&port, &threadsnum, &queuesize, schedalg, &max_size, argc, argv);

    // Initialize queues
    waiting_q = RequestQueue_create(queuesize);
    running_q = RequestQueue_create(queuesize);
    
    // Initialize and create thread pool
    pthread_t* ths = (pthread_t*)malloc(threadsnum*sizeof(*ths));

    for (int i=0; i<threadsnum; i++){
        if(pthread_create(&ths[i], NULL, &request_manager, (void*)&i)!=0){
            return 1;   //TODO: Error handling
        }
    }

    listenfd = Open_listenfd(port);

    //printf("Server: Starting to listen to incoming requests :)\n");

    bool add_to_waiting = true;

    while (1) {     // Server recieving requests

        
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        gettimeofday(&(th_stats.arrival), NULL);
        //printf("Server: Recieved a request\n");

        pthread_mutex_lock(&lock);
        while(RequestQueue_size(waiting_q) + RequestQueue_size(running_q) > queuesize){
            if (strcmp(schedalg, "block")==0){
                pthread_cond_wait(&cond_full, &lock);
                add_to_waiting = true;
                break;
            }
            else if (strcmp(schedalg, "drop_tail")==0){
                Close(connfd);
                add_to_waiting = false;
                break;
            }
            else if (strcmp(schedalg, "drop_head")==0){
                RequestQueue_dequeue(waiting_q, &err);
                // err = (waiting_q, connfd);
                // if (err!=QUEUE_SUCCESS){
                //     //Handle error;
                // }
                add_to_waiting = true;
                break;
            }
            else if (strcmp(schedalg, "block_flush")==0){
                while(RequestQueue_size(waiting_q) + RequestQueue_size(running_q) != 0){
                    pthread_cond_wait(&cond_full, &lock);
                }
                Close(connfd);
                add_to_waiting = false;
                break;
            }
            else if (strcmp(schedalg, "dynamic")==0){
                Close(connfd);
                if (RequestQueue_size(waiting_q) + RequestQueue_size(running_q) < max_size){
                    ++(waiting_q->capacity);
                }
                add_to_waiting = false;
                break;
            }
            else if (strcmp(schedalg, "drop_random")==0){
                RequestQueue_drop_half_random(waiting_q);
                add_to_waiting = true;
                break;
            }
        }
        pthread_mutex_unlock(&lock);

        if (!add_to_waiting){
            continue;
        }
        else{
            pthread_mutex_lock(&lock);        
            err = RequestQueue_queue(waiting_q, connfd, th_stats.arrival);
            pthread_mutex_unlock(&lock);
            
            if ( err!= QUEUE_SUCCESS ){
                //TODO: handle error

            }
            pthread_cond_signal(&cond_empty);
            // printf("Server: Forwarded request to threads\n");
        }
        
    }

    RequestQueue_destroy(waiting_q);
    RequestQueue_destroy(running_q);
    
    free(ths);
    
	// HW3: In general, don't handle the request in the main thread.
	// Save the relevant info in a buffer and have one of the worker threads 
	// do the work. 
	// 

    

}


    


 
