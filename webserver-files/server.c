#include <pthread.h>
#include <assert.h>

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
pthread_cond_t cond_th_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_main_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_main_empty = PTHREAD_COND_INITIALIZER;

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
    
    // pthread_mutex_lock(&lock);
    th_stats.th_id = *((int*)id);
    // pthread_mutex_unlock(&lock);
    free(id);
    // printf("Thread id: %d", th_stats.th_id);
    th_stats.th_stat_count = 0;
    th_stats.th_dyn_count = 0;
    th_stats.th_total_count = 0;

    while (1){
        
        pthread_mutex_lock(&lock);
        while (RequestQueue_isempty(waiting_q)){
            // printf("Thread: waiting for request\n");
        //UNLOCK - ?
            pthread_cond_wait(&cond_th_empty, &lock);
        }

        gettimeofday(&(th_stats.handle), NULL);

        //STATS: arrival time
        th_stats.arrival = RequestQueue_head_arrival(waiting_q, &err);
        
        //Dequeueing from waiting and enqueueing to running
        int req_fd = RequestQueue_dequeue(waiting_q, &err);   //Also deletes the req from waiting_q
        
        RequestQueue_queue(running_q, req_fd, th_stats.arrival);
        
        
        pthread_mutex_unlock(&lock);

        if (err != QUEUE_SUCCESS){
            //HANDLE ERROR
        }
        
        // ++(th_stats.th_total_count);

        //STATS: handle time
        // printf("Thread: Handling request\n");
        requestHandle(req_fd, &th_stats);
        Close(req_fd);
        //printf("Thread: handled and closed the request\n");

        pthread_mutex_lock(&lock);
        err = RequestQueue_dequeue_item(running_q, req_fd);
        //TODO: Check for errors
        pthread_cond_signal(&cond_main_full);

        if (RequestQueue_isempty(waiting_q) && RequestQueue_isempty(running_q)){
            pthread_cond_signal(&cond_main_empty);
        }
        pthread_mutex_unlock(&lock);
    }
    return NULL;
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
    if (strcmp(schedalg, "dynamic") == 0 && argc==6){
        *max_size = atoi(argv[5]);
    }
}


int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen, threadsnum, queuesize, max_size;
    char schedalg[13];
    struct sockaddr_in clientaddr;
    QueueError err;

    getargs(&port, &threadsnum, &queuesize, schedalg, &max_size, argc, argv);

    // Initialize queues
    waiting_q = RequestQueue_create(queuesize);
    running_q = RequestQueue_create(queuesize);
    
    // Initialize and create thread pool
    pthread_t* ths = (pthread_t*)malloc(threadsnum*sizeof(*ths));
    for (int i=0; i<threadsnum; i++){
        int* th_id = (int*)malloc(sizeof(*th_id));
        *th_id = i;
        pthread_create(&ths[i], NULL, request_manager, (void*)th_id);
        //TODO: Error handling
    
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
        if (RequestQueue_size(waiting_q) + RequestQueue_size(running_q) >= queuesize){
            
            if (strcmp(schedalg, "block")==0){
                while(RequestQueue_size(waiting_q) + RequestQueue_size(running_q) >= queuesize){
                    pthread_cond_wait(&cond_main_full, &lock);
                }
                add_to_waiting = true;
                // break;
            }
            else if (strcmp(schedalg, "dt")==0){
                Close(connfd);
                add_to_waiting = false;
                // pthread_mutex_unlock(&lock);
            }
            else if (strcmp(schedalg, "dh")==0){
                if (!RequestQueue_isempty(waiting_q)){
                    int fd_drop = RequestQueue_dequeue(waiting_q, &err);
                    Close(fd_drop);

                    add_to_waiting = true;
                    // break;
                }
                else{
                    Close(connfd);
                    add_to_waiting = false;
                    // pthread_mutex_unlock(&lock);

                }
            }
            else if (strcmp(schedalg, "bf")==0){

                while(RequestQueue_size(waiting_q) + RequestQueue_size(running_q) != 0){
                    pthread_cond_wait(&cond_main_empty, &lock);
                }
                Close(connfd);
                add_to_waiting = false;
                // pthread_mutex_unlock(&lock);
                // break;
            }
            else if (strcmp(schedalg, "dynamic")==0){
                Close(connfd);
                add_to_waiting = false;

                if (queuesize < max_size){
                    ++(waiting_q->capacity);
                    ++(running_q->capacity);
                    ++queuesize;
                }
                // pthread_mutex_unlock(&lock);

                // break;
            }
            else if (strcmp(schedalg, "random")==0){
                // RequestQueue_drop_half_random(waiting_q);
                if (RequestQueue_isempty(waiting_q)){
                    Close(connfd);
                    add_to_waiting = false;
                    // pthread_mutex_unlock(&lock);
                }
                else{
                    int end_num = (int)((RequestQueue_size(waiting_q))/2);
                    
                    while(RequestQueue_size(waiting_q) > end_num){
                        int* vals = RequestQueue_get_vals(waiting_q);
                        
                        if (vals != NULL){
                            int i_drop = rand()%(RequestQueue_size(waiting_q));
                            int fd_drop = vals[i_drop];
                            
                            err = RequestQueue_dequeue_item(waiting_q, fd_drop);
                            assert(err==QUEUE_SUCCESS);
                            
                            Close(fd_drop);
                            free(vals);
                        }
                        else{
                            break;
                        }
                    }
                    add_to_waiting = true;
                    // break;
                }                
            }
        }

        if (!add_to_waiting){
            pthread_mutex_unlock(&lock);
            continue;
        }
        
        
        err = RequestQueue_queue(waiting_q, connfd, th_stats.arrival);
        
        // assert(err==QUEUE_SUCCESS);

        pthread_cond_signal(&cond_th_empty);
        // printf("Server: Forwarded request to threads\n");
        pthread_mutex_unlock(&lock);
        
        
    }

    RequestQueue_destroy(waiting_q);
    RequestQueue_destroy(running_q);
    
    free(ths);    

}


    


 
