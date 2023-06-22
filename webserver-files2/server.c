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

pthread_mutex_t lock;
pthread_cond_t cond_th_empty;
pthread_cond_t cond_th_run_full;

pthread_cond_t cond_main_full;
pthread_cond_t cond_main_empty;

__thread ThreadStats th_stats;


void* request_manager(void* id){    //id is int
    QueueError err;
    
    th_stats.th_id = *((int*)id);
    free(id);
    th_stats.th_stat_count = 0;
    th_stats.th_dyn_count = 0;
    th_stats.th_total_count = 0;

    while (1){
        
        pthread_mutex_lock(&lock);
        while (RequestQueue_isempty(waiting_q)){
            pthread_cond_wait(&cond_th_empty, &lock);
        }

        //STATS: arrival time
        th_stats.arrival = RequestQueue_head_arrival(waiting_q, &err);

        //Dequeueing from waiting and enqueueing to running
        int req_fd = RequestQueue_dequeue(waiting_q, &err);   //Also deletes the req from waiting_q

        err = RequestQueue_queue(running_q, req_fd, th_stats.arrival);
        if (err!= QUEUE_SUCCESS){
            Close(req_fd);
            pthread_cond_signal(&cond_main_full);
            if (RequestQueue_isempty(waiting_q) && RequestQueue_isempty(running_q)){
                pthread_cond_signal(&cond_main_empty);
            }
            pthread_mutex_unlock(&lock);
            continue;
        }
        
        pthread_cond_signal(&cond_main_full);


        gettimeofday(&(th_stats.handle), NULL);
        pthread_mutex_unlock(&lock);

    
        //STATS: handle time
        requestHandle(req_fd, &th_stats);   //th_stats on stack
        
        pthread_mutex_lock(&lock);
        err = RequestQueue_dequeue_item(running_q, req_fd);
        if (err!= QUEUE_SUCCESS){
            Close(req_fd);
            pthread_mutex_unlock(&lock);
            continue;
        }
        Close(req_fd);

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
    char schedalg[7];
    struct sockaddr_in clientaddr;
    QueueError err;
    
    getargs(&port, &threadsnum, &queuesize, schedalg, &max_size, argc, argv);

    //Initialize lock, conds    
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond_th_empty, NULL);
    pthread_cond_init(&cond_th_run_full, NULL);
    pthread_cond_init(&cond_main_full, NULL);
    pthread_cond_init(&cond_main_empty, NULL);
    

    // Initialize queues
    waiting_q = RequestQueue_create(Max(threadsnum, queuesize-threadsnum));
    running_q = RequestQueue_create(threadsnum);
    
    // Initialize and create thread pool
    pthread_t* ths = (pthread_t*)malloc(threadsnum*sizeof(*ths));
    for (int i=0; i<threadsnum; i++){
        int* th_id = (int*)malloc(sizeof(*th_id));
        *th_id = i;
        pthread_create(&ths[i], NULL, request_manager, (void*)th_id);
    
    }

    listenfd = Open_listenfd(port);

    bool add_to_waiting = true;

    while (1) {     // Server recieving requests

        add_to_waiting = true;
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);


        struct timeval req_arrival;
        gettimeofday(&req_arrival, NULL);
        
        pthread_mutex_lock(&lock);
        if (RequestQueue_size(waiting_q) + RequestQueue_size(running_q) >= queuesize){
            
            if (strcmp(schedalg, "block")==0){
                while(RequestQueue_size(waiting_q) + RequestQueue_size(running_q) >= queuesize){
                    pthread_cond_wait(&cond_main_full, &lock);
                }
                add_to_waiting = true;
            }
            else if (strcmp(schedalg, "dt")==0){
                Close(connfd);
                add_to_waiting = false;
                pthread_mutex_unlock(&lock);

                continue;
            
            }
            else if (strcmp(schedalg, "dh")==0){
                if (RequestQueue_size(waiting_q) > 0){
                    int fd_drop = RequestQueue_dequeue(waiting_q, &err);
                    if (err != QUEUE_SUCCESS){
                        Close(connfd);
                        add_to_waiting = false;
                        pthread_mutex_unlock(&lock);
                        continue;
                    }
                    else{
                        Close(fd_drop);
                        add_to_waiting = true;

                    }

                }
                else{
                    Close(connfd);
                    add_to_waiting = false;
                    pthread_mutex_unlock(&lock);
                    continue;
            
                }
            }
            else if (strcmp(schedalg, "bf")==0){

                Close(connfd);
                add_to_waiting = false;
                while(RequestQueue_size(waiting_q) + RequestQueue_size(running_q) > 0){
                    pthread_cond_wait(&cond_main_empty, &lock);
                }
                
                pthread_mutex_unlock(&lock);
                continue;

            }
            else if (strcmp(schedalg, "dynamic")==0){
                Close(connfd);
                add_to_waiting = false;
                
                if (queuesize < max_size){
                    ++(waiting_q->capacity);
                    ++queuesize;
                }
                
                pthread_mutex_unlock(&lock);
                continue;
            
            }
            else if (strcmp(schedalg, "random")==0){
                if (RequestQueue_isempty(waiting_q)){
                    Close(connfd);
                    add_to_waiting = false;

                    pthread_mutex_unlock(&lock);
                    continue;
            
                }
                else{
                    int end_num = (int)((RequestQueue_size(waiting_q))/2);
                    
                    while(RequestQueue_size(waiting_q) > end_num){
                        int* vals = RequestQueue_get_vals(waiting_q);
                        
                        int fd_drop;
                        if (vals != NULL){
                            int i_drop = rand()%(RequestQueue_size(waiting_q));
                            fd_drop = vals[i_drop];
                            err = RequestQueue_dequeue_item(waiting_q, fd_drop);
                            Close(fd_drop);
                            free(vals);
                        }
                        else{
                            continue;
                        }

                    }
                    add_to_waiting = true;
                }                
            }
        }


        if(add_to_waiting){
            while(RequestQueue_size(waiting_q) == queuesize-threadsnum && queuesize!=threadsnum){
                pthread_cond_wait(&cond_main_full, &lock);
            }
            err = RequestQueue_queue(waiting_q, connfd, req_arrival);
            if (err != QUEUE_SUCCESS){
                Close(connfd);
            }
            else{
                pthread_cond_broadcast(&cond_th_empty);
            }
        }

        pthread_mutex_unlock(&lock);
        continue;
        
    }
    
    for (int i=0; i<threadsnum;i++){
        pthread_join(ths[i], NULL);
    }
    free(ths);  

    RequestQueue_destroy(waiting_q);
    RequestQueue_destroy(running_q);

}


    


 
