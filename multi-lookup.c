/*
 * File: multi-lookup.c
 * Author: Domenic Murtari
 * Project: CSCI 3753 Programming Assignment 2
 * Create Date: 2/23/2014
 * Modify Date: 2/23/2014
 * Description: Contains a multi-thread implementation of the DNS
 *  Lookup system
 * 
 */

#include "multi-lookup.h"

FILE* outputfp = NULL;

pthread_mutex_t queueMutex;
pthread_mutex_t outputMutex;
sem_t full;
sem_t empty;

void* requester(void* fileName){
	
	inputfp = fopen(argv[i], "r");
	if(!inputfp){
	    sprintf(errorstr, "Error Opening Input File: %s", argv[i]);
	    perror(errorstr);
	    break;
	}
	
	fclose(inputfp);	
}

void* resolver(){
	/* Read File and Process*/
	while(fscanf(inputfp, INPUTFS, hostname) > 0){

	    /* Lookup hostname and get IP string */
	    if(dnslookup(hostname, firstipstr, sizeof(firstipstr))
	       == UTIL_FAILURE){
			fprintf(stderr, "dnslookup error: %s\n", hostname);
			strncpy(firstipstr, "", sizeof(firstipstr));
	    }

	    /* Write to Output File */
	    fprintf(outputfp, "%s,%s\n", hostname, firstipstr);
	}
}

int main(int argc, char* argv[]){
    
    char hostname[SBUFSIZE];
    char errorstr[SBUFSIZE];
    char firstipstr[INET6_ADDRSTRLEN];
    int i;
    
    /* Check Arguments */
    if(argc < MINARGS){
		fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
		fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
		return EXIT_FAILURE;
    }

    /* Open Output File */
    outputfp = fopen(argv[(argc-1)], "w");
	    if(!outputfp){
		perror("Error Opening Output File");
		return EXIT_FAILURE;
    }

    /* Close Output File */
    fclose(outputfp);

    return EXIT_SUCCESS;
}
