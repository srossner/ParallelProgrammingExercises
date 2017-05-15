#include <pthread.h>
#include <stdlib.h>
#include <string.h>
//#include <stdio.h>
#include <ctype.h>
#include "histogram.h"
//static uint MAX_NAME_LENGTH;
//static uint MIN_NAME_LENGTH;
static int blocks_size_per_thread;
static int blocks_size_main;
typedef struct{
    int* histogram; //[NNAMES];
    block_t* blocks;
}threadData_t;
void countNames(int *nBlocks, block_t* blocks,  int* histogram)
{
    char current_word[20] = "";
    uint c = 0;
    for(int k = 0; k < NNAMES; k++)
    {
        histogram[k] = 0;
    }
    // loop over every character
    for (int i = 0; i < *nBlocks; i++) {
        for (int j = 0; j < BLOCKSIZE; j++) {
            // if the character is an alphabet store it to form a word.
            if(isalpha(blocks[i][j]) )// &&  c <= MAX_NAME_LENGTH)
            {
                current_word[c++] = blocks[i][j];
            }else {
                // if the character is not an alphabet it is the end of a word.
                // Compare the word with the list of names.
                //if( c <= MAX_NAME_LENGTH  || c >= MIN_NAME_LENGTH )
                {
                    current_word[c] = '\0';
                    for(int k = 0; k < NNAMES; k++)     {
                        if(!strcmp(current_word, names[k]))
                        {
                            histogram[k]++;
                        }
                    }
                }
                c = 0;
            }
        }
    }
}
void* worker(void *arg)
{
    threadData_t* threadData;
    threadData = (threadData_t*)arg;
    countNames(&blocks_size_per_thread, threadData->blocks, threadData->histogram );
    return NULL;
}
void get_histogram(int nBlocks,
                   block_t *blocks,
                   int* histogram,
                   int num_threads)
{
/*
    MAX_NAME_LENGTH = 0;
    MIN_NAME_LENGTH = 20;
    for(int k = 0; k < NNAMES; k++)
    {
        if(strlen(names[k]) > MAX_NAME_LENGTH)
            MAX_NAME_LENGTH = strlen(names[k]);
        if(strlen(names[k]) < MIN_NAME_LENGTH)
            MIN_NAME_LENGTH = strlen(names[k]);
    }
    */
    pthread_t* threads;
    threadData_t *threadsData;
    //memory allocation
    threads     = ( pthread_t* )    malloc( num_threads * sizeof( pthread_t )   );
    threadsData = ( threadData_t* ) malloc( num_threads * sizeof( threadData_t ));
    // calculate number of bloks per thread
    blocks_size_per_thread = (int) nBlocks/num_threads;
    //printf( "blocks_size_per_thread  %d\n", blocks_size_per_thread);
    //printf( "num_threads  %d\n", num_threads);

    // assingn blocks and blocks_size to thread Data
    for(int t=0; t<(num_threads-1); t++)
    {
        threadsData[t].blocks   = &blocks[blocks_size_per_thread * t];
        threadsData[t].histogram = (int*)malloc(NNAMES* sizeof(int));
    }
    // last thread gehts rest blocks on top
    blocks_size_main  = blocks_size_per_thread + nBlocks%num_threads;
    //printf( "blocks_size_main  %d\n", blocks_size_per_thread);
    threadsData[num_threads-1].blocks   = &blocks[blocks_size_per_thread *(num_threads-1)];
    threadsData[num_threads-1].histogram = histogram;
    int errorCode;
    for(int t=0; t<num_threads; t++){
       errorCode = pthread_create(&threads[t], NULL, worker, (void *)&threadsData[t]);
       if (errorCode){
          //printf("ERROR; return code from pthread_create() is %d\n", errorCode);
          exit(-1);
       }
    }
    //countNames( &blocks_size_main, threadsData[num_threads-1].blocks,  threadsData[num_threads-1].histogram);
    for(int t=0; t<num_threads-1; t++){
        pthread_join(threads[t], NULL);
        for(int k = 0; k < NNAMES; k++)
        {
            histogram[k] = histogram[k] + threadsData[t].histogram[k];
        }
        free(threadsData[t].histogram);
    }
    /*
    for(int k = 0; k < NNAMES; k++)
    {
        printf( "%s, %d \n", names[k], histogram[k]);
    }*/
    free(threads);
    free(threadsData);

    //printf( "\n\n\n");
}
