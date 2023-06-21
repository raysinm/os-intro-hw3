#include "segel.h"
#include "request.h"

// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//
// HW3: Parse the new arguments too





void getargs(int *port, int argc, char *argv[],int i)
{
    if (argc < 2) {
	fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	exit(1);
    }
    *port = atoi(argv[i]);

}

pthread_mutex_t m;
pthread_cond_t cond_pop;
pthread_cond_t cond_free_space;

    typedef struct _loud{              // this is the loud stract - it is the work to do the 
		int fd;                       // the socket to work on 
		struct timeval* startime;              // time of farst see (for part 3)
	}loud;
	
	
	typedef struct _box{               // this is the SYS the heart of the data structour
		loud** quee;                    // queue
		loud** tamp_quee;                  // queue
		int rp;                       // the index -ready to work on
		int ap;                       // the index- add to quee
		int size_of_quee;             // the cournt size of work in quee
		int max;                      // max size of queue (value by user)
		int thred_working;            // number of thread that are corently working (dont realy in use)
		
	}Box;
	
///////////////////////////////////////////////////////////////////////////////////////   
///////////////////////////////////data structor HELPER ///////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////// 

	void fill(int* array , int lenght , int num){
		int rd;
		while(num > 0){
		    rd=rand() % lenght;
		    if(array[rd] == -1){
				//printf("  %d  ",rd);
				num--;
				array[rd] = 1;
			} 
		}
		//printf("\n");
		return;
	}
	
	void bad_protocol_2(Box* box){
		int reall_size=box->size_of_quee - box->thred_working;
		int del = reall_size;
		if(del %2 == 1){
			del = del/2 +1;
		}else{
			del = del/2;
		}
		for(int i=0;i<del;i++){
		Close(box->quee[(box->rp)]->fd);
		free(box->quee[(box->rp)]->startime);
		free(box->quee[(box->rp)]);
	    box->rp=box->rp+1  % box->max;
	    box->size_of_quee--;
	    
		}
		return;
	}
	
	
	void protocol_2(Box* box){
		//printf("-------got into protocol_2-------\n");
		int reall_size=box->size_of_quee - box->thred_working;
		int del = reall_size;
		if(del %2 == 1){
			del = del/2+1 ;
		}else{
			del = del/2;
		}
		
		int* histo = (int*)malloc((reall_size)*sizeof(int));
		for(int i=0;i<reall_size;i++){
			histo[i]=-1;
		}
		//printf("the real size is %d and the del is %d\n",reall_size ,del);
		fill(histo, reall_size ,del);
		
		int new_index=0;
		for(int i=0 ; i< reall_size; i++){
			if(histo[i] == -1){
				box->tamp_quee[new_index] = box->quee[box->rp];
				box->rp=box->rp+1  % box->max;
				new_index++; ////////////////this might be it ,added the new_index = (new_index+1) % box->max,,,,,,befor was new_index++
			}else{
				//if(box->quee[(box->rp)%box->max] != NULL){
					//printf("i deleted: %d\n",i);
					Close(box->quee[(box->rp)]->fd);
					free(box->quee[(box->rp)]->startime);
					free(box->quee[(box->rp)]);
					box->rp=box->rp+1  % box->max;
				//}
			}	
		}
		box->rp =0;
		box->ap = new_index;
		box->size_of_quee= box->size_of_quee- del ;
		
		loud** ttt= box->quee;
		box->quee = box->tamp_quee;
		box->tamp_quee= ttt;
		//printf("-------the new ap is %d and the new rp is %d-------\n", box->ap , box->rp);
		if(histo != NULL){
			free(histo);
		}
	}
	
	void protocol(Box* box, int index){ // index can be: 1-drop_head    2-drop_random
		if(index == 1){
			loud* p= box->quee[box->rp];
			box->rp =((box->rp) + 1) % box-> max;
			(box->size_of_quee)--;
			
			Close(p->fd);
			if(p != NULL){
				free(p->startime);
				free(p);
			}
			return;
		}
		else{
			//printf("-----------------------------------2--------------------------------------\n");
			protocol_2(box);
			//printf("-----------------------------------3--------------------------------------\n");
			return;
		}
	}
	
	
	int translate(char* c){
		if(strcmp(c,"block") == 0){
			return 0;
		}
		if(strcmp(c,"dh") == 0){
			return 1;
		}
		if(strcmp(c,"random") == 0){
			return 2;
		}
		if(strcmp(c,"dt") == 0){
			return 3;
		}
		return 3;
	}
	
///////////////////////////////////////////////////////////////////////////////////////   
/////////////////////////////////// HELPER ////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////// 


///////////////////////////////////////////////////////////////////////////////////////   
///////////////////////////////////data structor///////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////  
// init        insialice the system
// add         add to que
// pop         give to a trhed a work from the quee 
// done        let the system know trhed finsh and deleit the job 
//             **its meen evry time the tread finish a work it messt call this founcsion befor taking a new woerk (pop)
	
	void init(Box* box,int nume_of_tread,int quee_len){
		loud** qp=(loud**)malloc(quee_len*sizeof(loud*));
		loud** qp_temp=(loud**)malloc(quee_len*sizeof(loud*));
		
		box->quee=qp;
		box->tamp_quee=qp_temp;
		box->thred_working=0;
		box->rp=0;
		box->ap=0;
		box->size_of_quee=0;
		box->max=quee_len;
		box->thred_working=0;
	
		return;	
	}
	
	
	void add(Box* box,loud* l,int protcol){//0-block    1-drop_head    2-drop_random     3-drop_tail
		pthread_mutex_lock(&m);//////////////////lock
		if(box->size_of_quee == box->max){
			//printf("-------got protocol %d-------\n", protcol);
			if(protcol == 0){ //block 
				while(box->size_of_quee == box->max){
					pthread_cond_wait(&cond_free_space,&m);////////////////wait for free space (only done() can signal)   ///WAS_FREE_SPACDE//
				}
			}else if(protcol==3){ //drop_tail
				Close(l->fd);
				if(l != NULL){
					free(l->startime);
					free(l);
				}
					
					pthread_mutex_unlock(&m);
					//pthread_cond_signal(&cond_pop);
					return;
			}else{
				//printf("-----------------------------------1--------------------------------------\n");
				protocol(box,protcol);
				//printf("-----------------------------------4--------------------------------------\n");
				
			}
		}
		box->quee[box->ap]=l;
		(box->size_of_quee)++;
		box->ap = ((box->ap) + 1) % box->max;
		
		pthread_cond_broadcast(&cond_pop);//////////////signel all sleping tread the they can take frome quee   broadcast
		pthread_mutex_unlock(&m);////////////////unlock
		//pthread_cond_signal(&cond_pop);
		return;
	}
	
	// ./client loclhost 8080 output.cgi?1
	
	loud* pop(Box* box,int t_id){
		pthread_mutex_lock(&m);//////////////////lock
		//printf("took a job an my id is %d\n",t_id);
		while(box->size_of_quee - box->thred_working == 0){
           pthread_cond_wait(&cond_pop,&m);////////////////wait for new job (only add() can signal)
	    }
		loud* r =box->quee[box->rp];
		box->rp = (box->rp+1) % box->max;     
		(box->thred_working)++;
		pthread_mutex_unlock(&m);////////////////unlock
		//printf("the size is %d  ,the ap is %d  ,the rp is %d  ,the max is %d\n",box->size_of_quee , box->ap , box->rp,box->max);
		return r;
	}
	
	void done(Box* box ,int t_id){
		
		pthread_mutex_lock(&m);//////////////////lock
		
		(box->thred_working)--;
		(box->size_of_quee)--;
		if(box->size_of_quee==box->max -1){
			pthread_cond_signal(&cond_free_space); ////////////// signal that ther is free place in quee (for the man trhed) //cond_free_space
		}
		pthread_mutex_unlock(&m);////////////////unlock
		return;
	}
	
	
///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// end of data structur///////////////////////////////////// 
///////////////////////////////////////////////////////////////////////////////////////  
Box SYS;



void* treadFouncsion(void* in){ // walcome to the thread ---- the thread founcsion
	 int index = *((int*)in);
	 int count=0;
	 int count_static=0;
	 int count_dynamic=0;
	 loud* work;
	 struct timeval new_time;
	 while(1){
		 work = pop(&SYS,index);// take work frome quee or wait
		
		 count++;
		 gettimeofday(&new_time , NULL);
		 time_t sec= new_time.tv_sec -work->startime->tv_sec;
		 suseconds_t  subs=new_time.tv_usec -work->startime->tv_usec;
		 if(subs < 0){
		 sec--;
		 subs = 1000000+subs;
		 }
		 new_time.tv_sec=sec;
		 new_time.tv_usec=subs;
		 requestHandle(work->fd ,&count, &count_static , &count_dynamic, work->startime ,&new_time, index);
		 
		 done(&SYS,index);    // updat sys(data structor) that work is don	
		 Close(work->fd);
		 free(work->startime);
		 free(work);
		//printf("end a job and my id is %d\n",index);
		 	 
	 }	 
 }

int main(int argc, char *argv[])
{
	pthread_mutex_init(&m,NULL);
	pthread_cond_init(&cond_pop,NULL);
	pthread_cond_init(&cond_free_space,NULL);
	
	int num_treads;
	int quee_len;
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;
    
    getargs(&port, argc, argv ,1);
    getargs(&num_treads, argc, argv, 2);
    getargs(&quee_len, argc, argv, 3);
    int schedalg =translate(argv[4]);
    
    init(&SYS,num_treads,quee_len); // insial data structour start system;
    
    pthread_t* trads=(pthread_t*)malloc(num_treads*sizeof(pthread_t));//creating the pool of tread;
    int* arry_num =(int*)malloc(num_treads*sizeof(int));
    for(int i=0 ; i<num_treads;i++){
		arry_num[i]=i;
		pthread_create(&trads[i],NULL,treadFouncsion,(void*)(arry_num+i));
	}

    listenfd = Open_listenfd(port);
    while (1) {
		//sleep(10);
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
		
		loud* new_woark = (loud*)malloc(sizeof(loud));
		struct timeval* tv= (struct timeval*)malloc(sizeof(struct timeval));
		gettimeofday(tv,NULL);
		new_woark->startime = tv;
		new_woark->fd=connfd;
		add(&SYS,new_woark,schedalg); //add to quee
		
    }
}






/*
 	void add(Box* box,loud* l,int protcol){//0-block    1-drop_head    2-drop_random     3-drop_tail
		pthread_mutex_lock(&m);//////////////////lock
		if(box->size_of_quee == box->max){
			if(protcol==0){
				while(box->size_of_quee == box->max){
					pthread_cond_wait(&cond_free_space,&m);////////////////wait for free space (only done() can signal)   ///WAS_FREE_SPACDE//
				}
			}else if(protcol==3){
				Close(l->fd);
				if(l != NULL){
					free(l->startime);
					free(l);
				}
					pthread_mutex_unlock(&m);
					return;
			}else{
				protocol(box,protcol);
				
			}
		}
		box->quee[box->ap]=l;
		(box->size_of_quee)++;
		box->ap = ((box->ap) + 1) % box->max;
		//printf("the size is %d  ,the ap is %d  ,the rp is %d  ,the max is %d\n",box->size_of_quee , box->ap , box->rp,box->max);
		pthread_cond_broadcast(&cond_pop);//////////////signel all sleping tread the they can take frome quee
		pthread_mutex_unlock(&m);////////////////unlock
		return;
	}
*/
