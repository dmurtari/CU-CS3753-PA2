/*
 * File: multi-lookup.c
 * Author: Domenic Murtari
 * Project: CSCI 3753 Programming Assignment 2
 * Create Date: 2/23/2014
 * Modify Date: 2/27/2014
 * Description: Contains a multi-thread implementation of the DNS Lookup system
 * 
 * References: 
 *  http://jlmedina123.wordpress.com/2013/05/03/pthreads-with-mutex-and-semaphores/
 *  http://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine
 * 
 */

#include "multi-lookup.h"

FILE* outputfp;             // Pointer to the output file
queue q;                    // Queue to store hostnames in
int runningRequesters = 0;  // Count of the number of running requestors threads

/* Mutexes to control access to shared resources */
pthread_mutex_t queueMutex;
pthread_mutex_t outputMutex;
pthread_mutex_t requesterMutex;

/*
 * Function for the requestor threads. Takes a pointer to a file as input, and 
 * goes through that file inserting hostnames into the queue. Waits for a 
 * random length of time if the queue is full.
 */
void* requester(void* fileName){
    
  FILE* inputfp;
  char errorstr[SBUFSIZE];
  char hostname[MAX_NAME_LENGTH];
  char* payload;
  struct timespec reqtime;
  
  /* Open the input file for import */  
  inputfp = fopen(fileName, "r");
  if(!inputfp){
    sprintf(errorstr, "Error Opening Input File: %p", fileName);
    perror(errorstr);
  }

  /* Go through the input file, inserting hostnames into the queue */
  while(fscanf(inputfp, INPUTFS, hostname) > 0){
    
    /* Acquire queue mutex lock so other requestors can't insert at same time 
     * and resolvers can't try to read from the queue */
    pthread_mutex_lock(&queueMutex);

    /* Allocate memory for payload and copy to array */
    payload = (char*)malloc(sizeof(hostname));
    strcpy(payload, hostname);

    /* Sleep for a random length of time if queue is full */
    while(queue_is_full(&q)){
      pthread_mutex_unlock(&queueMutex);
      reqtime.tv_sec = 0;
      reqtime.tv_nsec = rand() % 100;
      nanosleep(&reqtime, NULL);
      pthread_mutex_lock(&queueMutex);
    }

    /* There is room in the queue, so add new hostname to queue */
    if(queue_push(&q, (void*)payload) == QUEUE_FAILURE)
      fprintf(stderr, "Error: Queue push failed\n");

    /* Unlock queue for other requestors and resolvers to be able to read*/
    pthread_mutex_unlock(&queueMutex);
  }
  
  /* Done processing file, so requester thread will terminate. Make sure that no
   * other requestors can access the counting variable at the same time */
  pthread_mutex_lock(&requesterMutex);
  runningRequesters--;
  pthread_mutex_unlock(&requesterMutex);

  /* Close input file and return */
  fclose(inputfp);  
  return NULL;
}   
    
/*
 * Function for the resolver threads. Reads from the queue that the requesters
 * populate, looking up the hostname and writing the result to the output file.
 */
void* resolver(){
    
  char* hostname;
  char firstipstr[INET6_ADDRSTRLEN];

  /* While there are still requesters running and there are still items in the
   * queue, try to read items from the queue and resolve the hostnames */
  while(1){

    /* Check to see if queue is empty and that there are no more requesters
     * waiting to complete. Need to control access to the queue and the count
     * of running requesters to do this. If the queue is empty and no requesters
     * are running, exit the while loop */
    pthread_mutex_lock(&queueMutex);
    pthread_mutex_lock(&requesterMutex);
    if(queue_is_empty(&q) && (runningRequesters == 0)){
      pthread_mutex_unlock(&queueMutex);
      pthread_mutex_unlock(&requesterMutex);
      break;
    }

    /* Don't need access to the count of running requesters anymore */
    pthread_mutex_unlock(&requesterMutex);

    /* If queue is empty, no need to try and look anything up so continue to the
     * next iteration */
    if(queue_is_empty(&q)){
      pthread_mutex_unlock(&queueMutex);
      continue;
    }
    
    /* There is something in the queue, so read a name from the queue */
    hostname = (char*)queue_pop(&q);

    /* Don't need exclusive access to the queue anymore */
    pthread_mutex_unlock(&queueMutex);

    /* Lookup hostname */
    if(dnslookup(hostname, firstipstr, sizeof(firstipstr)) == UTIL_FAILURE){
      fprintf(stderr, "dnslookup error: %s\n", hostname);
      strncpy(firstipstr, "", sizeof(firstipstr));
    }

    /* Write result to the output file. Need exclusive access to output */
    pthread_mutex_lock(&outputMutex);
    fprintf(outputfp, "%s,%s\n", hostname, firstipstr);
    pthread_mutex_unlock(&outputMutex);

    /* Free memory used by hostname */
    free(hostname);
  }

  return NULL;
}


int main(int argc, char* argv[]){
    
  int i;
  /* Number of requester threads is number of input files */
  int requesterThreadCount = argc - 2;
  /* Number of resolver threads is the number of cores */
  int resolverThreadCount = 10; //sysconf( _SC_NPROCESSORS_ONLN );
  /* Set number of running requesters to the numbers of input files */
  runningRequesters = requesterThreadCount;
  
  
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
    return EXIT_FAILURE;
  }

  /* Open Output File */
  outputfp = fopen(argv[(argc-1)], "w");
    if(!outputfp){
      perror("Error Opening Output File\n");
      return EXIT_FAILURE;
  }

  /* Create the queue, based on QUEUE_SIZE defined in header file */
  if(queue_init(&q, QUEUE_SIZE) == QUEUE_FAILURE)
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
  if(pthread_mutex_init(&requesterMutex, NULL)){
    fprintf(stderr, "Error: requesterMutex initialization failed\n");
    return EXIT_FAILURE;
  }

  /* Create thread pools */
  pthread_t requesterThreads[requesterThreadCount];
  pthread_t resolverThreads[resolverThreadCount];
  
  /* Populate thread pools with threads */
  for(i = 0; i < requesterThreadCount; i++){
    if(pthread_create(&requesterThreads[i], NULL, requester, argv[i + 1])){
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
  pthread_mutex_destroy(&requesterMutex);
  queue_cleanup(&q);

  return EXIT_SUCCESS;
}
