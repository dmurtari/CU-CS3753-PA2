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
 * 
 */

#include "multi-lookup.h"

FILE* outputfp;             // Pointer to the output file
queue q;                    // Queue to store hostnames in
int runningRequesters = 0;  // Count of the number of running requesters threads

/* Mutexes to control access to shared resources */
pthread_mutex_t queueMutex;     // Mutex for access to the queue
pthread_mutex_t outputMutex;    // Mutex for access to the output file
pthread_mutex_t requesterMutex; // Mutex for the running requesters thread count

/* Semaphores to communicate between threads */
sem_t full;   // Semaphore to see if there is something in the queue
sem_t empty;  // Semaphore to count the number of empty spaces in queue

/*
 * Function for the requester threads. Takes a pointer to a file as input, and 
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
    
    /* Wait until there is an empty slot in the queue */
    sem_wait(&empty);

    /* Allocate memory for payload and copy to array */
    payload = (char*)malloc(sizeof(hostname));
    strcpy(payload, hostname);

    /* Acquire queue mutex lock so other requesters can't insert at same time 
     * and resolvers can't try to read from the queue */
    pthread_mutex_lock(&queueMutex);

    /* Add an item to the queue. If the queue is full and returns failure, sleep
     * and try again after a random time */
    while(queue_push(&q, (void*)payload) == QUEUE_FAILURE){
      pthread_mutex_unlock(&queueMutex);
      reqtime.tv_sec = 0;
      reqtime.tv_nsec = rand() % 100;
      nanosleep(&reqtime, NULL);
      pthread_mutex_lock(&queueMutex);
    }

    /* Unlock queue and signal that there is something in the queue */
    pthread_mutex_unlock(&queueMutex);
    sem_post(&full);
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

    /* Unlock mutexes in case this thread has to wait on full */
    pthread_mutex_unlock(&queueMutex);
    pthread_mutex_unlock(&requesterMutex);

    /* Wait for there to be something in the queue */
    sem_wait(&full);

    /* There is something in the queue, so read a name from the queue */
    pthread_mutex_lock(&queueMutex);
    hostname = (char*)queue_pop(&q);
    pthread_mutex_unlock(&queueMutex);

    /* Signal that there is room in the queue */
    sem_post(&empty);

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
<<<<<<< HEAD
  int resolverThreadCount = sysconf( _SC_NPROCESSORS_ONLN );
=======
  int resolverThreadCount = sysconf(_SC_NPROCESSORS_ONLN) * 2;
>>>>>>> 575174d751012770fc53a406e30f76c80daec1e1
  /* Set number of running requesters to the numbers of input files */
  runningRequesters = requesterThreadCount;

  struct timeval startTime;
  struct timeval endTime;
  long elapsedTime;
  
  
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

  /* Initialize semaphores */
  if(sem_init(&empty, 0, QUEUE_SIZE)){
    fprintf(stderr, "Error: empty Semaphore initialization failed\n");
    return EXIT_FAILURE;
  }
  if(sem_init(&full, 0, 0)){
    fprintf(stderr, "Error: full Semaphore initialization failed\n");
  }

  gettimeofday(&startTime, NULL);

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
  for(i = 0; i < requesterThreadCount; i++){
    if(pthread_join(requesterThreads[i], NULL)){
      fprintf(stderr, "Error: Joining requester thread %d failed\n", i);
    }
  }
  for(i = 0; i < resolverThreadCount; i++){
    if(pthread_join(resolverThreads[i], NULL)){
      fprintf(stderr, "Error: Joining resolver thread %d failed\n", i);
    }
  }
  
  gettimeofday(&endTime, NULL);

  /* Close Output File */
  fclose(outputfp);

  /* Cleanup */
  if(pthread_mutex_destroy(&queueMutex))
    fprintf(stderr, "Error: Destroying queueMutex failed\n");
  if(pthread_mutex_destroy(&outputMutex))
    fprintf(stderr, "Error: Destroying outputMutex failed\n");
  if(pthread_mutex_destroy(&requesterMutex))
    fprintf(stderr, "Error: Destroying requesterMutex failed\n");
  if(sem_destroy(&full))
    fprintf(stderr, "Error: Destroying full semaphore failed\n");
  if(sem_destroy(&empty))
    fprintf(stderr, "Error: Destroying empty semaphore failed\n");
  queue_cleanup(&q);

  elapsedTime = endTime.tv_usec - startTime.tv_usec;
  printf("Elapsed time was: %ld\n", elapsedTime);

  return EXIT_SUCCESS;
}
