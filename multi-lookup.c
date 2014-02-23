/*
 * File: multi-lookup.c
 * Author: Domenic Murtari
 * Project: CSCI 3753 Programming Assignment 2
 * Create Date: 2/23/2014
 * Modify Date: 2/23/2014
 * Description: Contains a multi-thread implementation of the DNS Lookup system
 * 
 * References: 
 *  http://jlmedina123.wordpress.com/2013/05/03/pthreads-with-mutex-and-semaphores/
 *  http://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine
 * 
 */

#include "multi-lookup.h"

FILE* outputfp = NULL;
queue q;

pthread_mutex_t queueMutex;
pthread_mutex_t outputMutex;
sem_t full;
sem_t empty;

void* requester(void* fileName){
    
  FILE* inputfp = NULL;
  char errorstr[SBUFSIZE];
  char hostname[MAX_NAME_LENGTH];
  char* payload;
    
  inputfp = fopen(fileName, "r");
  if(!inputfp){
    sprintf(errorstr, "Error Opening Input File: %s", argv[i]);
    perror(errorstr);
  }

  while(fscanf(inputf, INPUTFS, hostname) > 0){
    /* Acquire queue mutex lock, and empty semaphore lock */
    pthread_mutex_lock(&queueMutex);
    sem_wait(&empty);

    /* Allocate memory for payload and copy to array */
    payload = (char*)malloc(sizeof(hostname));
    strcpy(payload, hostname);

    /* Add new hostname to queue */
    if(queue_push(&q, payload_in[i]) == QUEUE_FAILURE)
      fprintf(stderr, "error: queue_push failed!");

    /* Unlock queue, signal full */
    pthread_mutex_unlock(&queueMutex);
    sem_signal(&full);
  }
  
  /* Close input file and return */
  fclose(inputfp);  
  return NULL
}   
    
void* resolver(){
    
  char hostname[SBUFSIZE];
  char firstipstr[INET6_ADDRSTRLEN];
    
  /* Read File and Process*/
  while(fscanf(inputfp, INPUTFS, hostname) > 0){
    /* Lookup hostname and get IP string */
    if(dnslookup(hostname, firstipstr, sizeof(firstipstr)) == UTIL_FAILURE){
        fprintf(stderr, "dnslookup error: %s\n", hostname);
        strncpy(firstipstr, "", sizeof(firstipstr));
    }

    /* Write to Output File */
    fprintf(outputfp, "%s,%s\n", hostname, firstipstr);
  }
}

int main(int argc, char* argv[]){
    
    int i;
    /* Number of requester threads is number of input files */
    int requesterThreadCount = argc - 2;
    /* Number of resolver threads is the number of cores */
    int resolverThreadCount = sysconf(_SC_NPROCESSORS_ONLN);
    
    /* Check Arguments */
    if(argc < MINARGS){
      fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
      fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
      return EXIT_FAILURE;
    }
    if(resolverThreadCount < MIN_RESOLVER_THREADS){
      fprintf(stderr, "Not enough resolver threads: %d\n", 
              resolverThreadCount);
      fprintf(stderr, "Requires %d threads", MIN_RESOLVER_THREADS);
      return EXIT_FAILURE:
    }

    /* Open Output File */
    outputfp = fopen(argv[(argc-1)], "w");
      if(!outputfp){
        perror("Error Opening Output File");
        return EXIT_FAILURE;
    }

    /* Create the queue, based on QUEUE_SIZE defined in header file */
    if(queue_init(&queue, QUEUE_SIZE) == QUEUE_FAILURE)
      fprintf(stderr,"Error: queue_init failed!\n");

    /* Initialize mutexes */
    if(pthread_mutex_init(&queueMutex, NULL)){
      fprintf(stderr, "Error: queueMutex initialization failed\n");
      return EXIT_FAILURE;
    }
    if(pthread_mutex_init(&outputMutex, NULL)){
      fprintf(stderr, "Error: outputMutex initialization failed\n");
      return EXIT_FAILURE;
    }
    
    /* Initialize semaphores */
    if(sem_init(&full, 0, 0)){
      fprintf(stderr, "Error: full semaphore initialization failed\n");
      return EXIT_FAILURE;
    }
    if(sem_init(&empty, 0, QUEUE_SIZE)){
      fprintf(stderr, "Error: full semaphore initialization failed\n");
      return EXIT_FAILURE;
    }

    /* Create thread pools */
    pthread_t requesterThreads[requesterThreadCount];
    pthread_t resolverThreads[resolverThreadCount];
    
    /* Create threads */
    for(i = 0; i < requesterThreadCount; i++){
      if(pthread_create(&requesterThreads[i], NULL, requester, argv[i + 1]){
        fprintf(stderr, "Error: Creating requester threads failed\n");
        return EXIT_FAILURE;
      }
    }
    for(i = 0; i < resolverThreadCount; i++){
      if(pthread_create(&resolverThreads[i], NULL, resolver, NULL)){
        fprintf(stderr, "Error: Creating resolver threads failed\n");
        return EXIT_FAILURE;
      }
    }

    /* Wait for requester and resolver threads to both finish */
    for(i = 0; i < requesterThreadCount; i++)
      pthread_join(requesterThreads[i], NULL);
    for(i = 0; i < resolverThreadCount; i++)
      pthread_join(resolverThreads[i], NULL);
    
    /* Close Output File */
    fclose(outputfp);

    /* Cleanup */
    pthread_mutex_destroy(&queueMutex);
    pthread_mutex_destroy(&outputMutex);
    sem_destroy(&full);
    sem_destroy(&empty);
    queue_cleanup(&queue);

    return EXIT_SUCCESS;
}
