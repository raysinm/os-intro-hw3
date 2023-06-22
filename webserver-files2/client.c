/*
 * client.c: A very, very primitive HTTP client.
 * 
 * To run, try: 
 *      ./client www.cs.technion.ac.il 80 /
 *
 * Sends one HTTP request to the specified HTTP server.
 * Prints out the HTTP response.
 *
 * HW3: For testing your server, you will want to modify this client.  
 * For example:
 * 
 * You may want to make this multi-threaded so that you can 
 * send many requests simultaneously to the server.
 *
 * You may also want to be able to request different URIs; 
 * you may want to get more URIs from the command line 
 * or read the list from a file. 
 *
 * When we test your server, we will be using modifications to this client.
 *
 */

#include <pthread.h>

#include "segel.h"

/*
 * Send an HTTP request for the specified file 
 */
void clientSend(int fd, char *filename)
{
  char buf[MAXLINE];
  char hostname[MAXLINE];

  Gethostname(hostname, MAXLINE);

  /* Form and send the HTTP request */
  sprintf(buf, "GET %s HTTP/1.1\n", filename);
  sprintf(buf, "%shost: %s\n\r\n", buf, hostname);
  Rio_writen(fd, buf, strlen(buf));
}
  
/*
 * Read the HTTP response and print it out
 */
void clientPrint(int fd)
{
  rio_t rio;
  char buf[MAXBUF];  
  int length = 0;
  int n;
  
  Rio_readinitb(&rio, fd);

  /* Read and display the HTTP Header */
  n = Rio_readlineb(&rio, buf, MAXBUF);
  while (strcmp(buf, "\r\n") && (n > 0)) {
    printf("Header: %s", buf);
    n = Rio_readlineb(&rio, buf, MAXBUF);

    /* If you want to look for certain HTTP tags... */
    if (sscanf(buf, "Content-Length: %d ", &length) == 1) {
      printf("Length = %d\n", length);
    }
  }

  /* Read and display the HTTP Body */
  n = Rio_readlineb(&rio, buf, MAXBUF);
  while (n > 0) {
    printf("%s", buf);
    n = Rio_readlineb(&rio, buf, MAXBUF);
  }
}


char *host, *filename;
int port;

void* client_routine(){
  
  int clientfd;
  // int clientfd = *(int*)fd;
  // free(fd); 

  
  /* Open a single connection to the specified host and port */
  clientfd = Open_clientfd(host, port);
  
  // printf("Client: Starting to send a request :)\n");

  clientSend(clientfd, filename);

  // printf("Client: Returned from the request\n");
  
  clientPrint(clientfd);

  // printf("Client: Returned from clientPrint\n");
  
  Close(clientfd);

  pthread_exit(NULL);

}

int main(int argc, char *argv[])
{
  
  int num_threads = 1;

  if (argc < 4 || argc > 6) {
    fprintf(stderr, "Usage: %s <host> <port> <filename> <num_client_threads>(Optional)\n", argv[0]);
    exit(1);
  }

  host = argv[1];
  port = atoi(argv[2]);
  filename = argv[3];

  if (argc==5){
    num_threads = atoi(argv[4]);
  }

  pthread_t* ths = (pthread_t*)malloc(sizeof(*ths)*num_threads); 

  for (int i=1; i<num_threads+1; i++){
    // int* fd = (int*)malloc(sizeof(int));
    // *fd = i;
    pthread_create(&ths[i], NULL, client_routine, NULL);
    // free(fd);
  } 

  for (int i=1; i<num_threads+1; i++){
    pthread_join(ths[i],NULL);
  }
  // printf("Client: Returned from Close\n");
  
  free(ths);

  exit(0);
}
