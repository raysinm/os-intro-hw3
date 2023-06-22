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

// FILE* debug;

void* request_manager(void* id){    //id is int
    QueueError err;
    
    th_stats.th_id = *((int*)id);
    free(id);
    th_stats.th_stat_count = 0;
    th_stats.th_dyn_count = 0;
    th_stats.th_total_count = 0;

    while (1){
        
        pthread_mutex_lock(&lock);
        // FILE *file = fopen("queue_errs.txt", "a");
        while (RequestQueue_isempty(waiting_q)){
            // printf("Thread: waiting for request\n");
        //UNLOCK - ?
            pthread_cond_wait(&cond_th_empty, &lock);
        }

        while(running_q->size >= running_q->capacity){
            pthread_cond_wait(&cond_th_run_full, &lock);
        }

        if(RequestQueue_isempty(waiting_q)){
            pthread_mutex_unlock(&lock);
            continue;
        }

        //STATS: arrival time
        th_stats.arrival = RequestQueue_head_arrival(waiting_q, &err);
        // fprintf(file,"%d\n", err);
        //Dequeueing from waiting and enqueueing to running
        int req_fd = RequestQueue_dequeue(waiting_q, &err);   //Also deletes the req from waiting_q
        
        
        // fprintf(file,"%d\n", err);
        err = RequestQueue_queue(running_q, req_fd, th_stats.arrival);
        // fprintf(file,"%d\n", err);
        // debug = fopen("debug.txt", "a");
    
        // fprintf(debug, "Thread Handles fd: %d\n", req_fd);
        // fprintf(debug, "Waiting q:");

        // int* vals = RequestQueue_get_vals(waiting_q);
        // for (int i=0; i< RequestQueue_size(waiting_q); i++){
        //     fprintf(debug, " %d,", vals[i]);
        // }
        // free(vals);
        // fprintf(debug, "\n");

        // fprintf(debug, "Running q:");

        // vals = RequestQueue_get_vals(running_q);
        // for (int i=0; i< RequestQueue_size(running_q); i++){
        //     fprintf(debug, " %d,", vals[i]);
        // }
        // free(vals);
        // fprintf(debug, "\n");
        // fclose(debug);
        // fclose(file);
        gettimeofday(&(th_stats.handle), NULL);
        pthread_mutex_unlock(&lock);

        
        // ++(th_stats.th_total_count);

        //STATS: handle time
        // printf("Thread: Handling request\n");
        requestHandle(req_fd, &th_stats);   //th_stats on stack
        
        pthread_mutex_lock(&lock);
        //printf("Thread: handled and closed the request\n");

        // file = fopen("queue_errs.txt", "a");
        if (RequestQueue_isempty(running_q)){
            
        }
        err = RequestQueue_dequeue_item(running_q, req_fd);
        Close(req_fd);
        pthread_cond_signal(&cond_th_run_full);
        // debug = fopen("debug.txt", "a");
        // fprintf(debug, "Thread Finished fd: %d\n", req_fd);
        // fclose(debug);
    
        //TODO: Check for errors
        pthread_cond_signal(&cond_main_full);

        if (RequestQueue_isempty(waiting_q) && RequestQueue_isempty(running_q)){
            pthread_cond_signal(&cond_main_empty);
        }
        pthread_mutex_unlock(&lock);
        
    }
    pthread_exit(NULL);
    // return NULL;
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
    
    // debug = fopen("debug.txt", "a");

    getargs(&port, &threadsnum, &queuesize, schedalg, &max_size, argc, argv);

    //Initialize lock, conds    
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond_th_empty, NULL);
    pthread_cond_init(&cond_th_run_full, NULL);
    pthread_cond_init(&cond_main_full, NULL);
    pthread_cond_init(&cond_main_empty, NULL);
    

    // Initialize queues
    waiting_q = RequestQueue_create(queuesize-threadsnum);
    running_q = RequestQueue_create(threadsnum);
    
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



    int random_debug = 0;
    int vals_null_debug = 0;
    int full_debug = 0;
    int accepts_debug = 0;
    while (1) {     // Server recieving requests
        //DEBUG FILE:
        // FILE *file = fopen("end_num.txt", "a");
    
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);


        struct timeval req_arrival;
        gettimeofday(&req_arrival, NULL);
        
        ++accepts_debug;
        // debug = fopen("debug.txt", "a");
        // fprintf(debug, "Accpted fd: %d\n", connfd);
        // fprintf(debug, "Waiting q:");

        // int* vals = RequestQueue_get_vals(waiting_q);
        // for (int i=0; i< RequestQueue_size(waiting_q); i++){
        //     fprintf(debug, " %d,", vals[i]);
        // }
        // free(vals);
        // fprintf(debug, "\n");
        // fclose(debug);

        // if (file){
        //     fprintf(file, "\nTotal accepts: %d\tfd: %d\n", accepts_debug, connfd);
        // }
        //printf("Server: Recieved a request\n");

        pthread_mutex_lock(&lock);
        if (RequestQueue_size(waiting_q) + RequestQueue_size(running_q) >= queuesize){
            ++full_debug;
            // if(file!=NULL){
            //     fprintf(file, "Num of full times: %d\n", full_debug);
            // }
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
                pthread_mutex_unlock(&lock);

                continue;
            
                // pthread_mutex_unlock(&lock);
            }
            else if (strcmp(schedalg, "dh")==0){
                if (RequestQueue_size(waiting_q) > 0){
                    int fd_drop = RequestQueue_dequeue(waiting_q, &err);
                    Close(fd_drop);
                    // debug = fopen("debug.txt", "a");

                    // fprintf(debug, "Dropped %d\n", fd_drop);
                    // fclose(debug);
                    add_to_waiting = true;
                    // break;
                }
                else{
                    Close(connfd);
                    add_to_waiting = false;
                    // debug = fopen("debug.txt", "a");
                    // fprintf(debug, "Dropped current req");
                    // fclose(debug);

                    pthread_mutex_unlock(&lock);
                    continue;
            
                    // pthread_mutex_unlock(&lock);
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
            
                // pthread_mutex_unlock(&lock);
                // break;
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
            

                // pthread_mutex_unlock(&lock);

                // break;
            }
            else if (strcmp(schedalg, "random")==0){
                random_debug++;
                // RequestQueue_drop_half_random(waiting_q);
                if (RequestQueue_isempty(waiting_q)){
                    Close(connfd);
                    add_to_waiting = false;

                    pthread_mutex_unlock(&lock);
                    continue;
            
                    // pthread_mutex_unlock(&lock);
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
                            // assert(err==QUEUE_SUCCESS);
                            
                            Close(fd_drop);
                            free(vals);
                        }
                        else{
                            ++vals_null_debug;
                            continue;
                        }
                    // FILE *file = fopen("end_num.txt", "a");
                    // if (file!=NULL){
                    //     fprintf(file, "Random entries: %d\tend_num: %d\tWaiting q size: %d\tRunning q size:%d\tfd_drop: %d\n", random_debug, end_num, RequestQueue_size(waiting_q), RequestQueue_size(running_q), fd_drop);
                    //     fprintf(file, "vals_null: %d\n", vals_null_debug);
                    // }
                    }
                    add_to_waiting = true;
                    // break;
                }                
            }
        }


        // if (!add_to_waiting){
        //     pthread_mutex_unlock(&lock);
        //     continue;
        //     // if (file){
        //     //     fclose(file);
        //     // }            
        // }
        if(add_to_waiting){
            err = RequestQueue_queue(waiting_q, connfd, req_arrival);
            // debug = fopen("debug.txt", "a");
            // fprintf(debug, "Main pushed fd: %d\n", connfd);
            // // assert(err==QUEUE_SUCCESS);
            // fclose(debug);
            pthread_cond_broadcast(&cond_th_empty);
            // printf("Server: Forwarded request to threads\n");
            pthread_mutex_unlock(&lock);

        }
        else{
            pthread_mutex_unlock(&lock);
            continue;
        }
        
        
        
        // if (file){
        //     fclose(file);
        // }
    }
    
    for (int i=0; i<threadsnum;i++){
        pthread_join(ths[i], NULL);
    }
    free(ths);  

    RequestQueue_destroy(waiting_q);
    RequestQueue_destroy(running_q);

    // fclose(debug);

}


    


 
