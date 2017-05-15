#include "histogram.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "names.h"


typedef struct{
    int* histogram; //[NNAMES];
    block_t* blocks;
}threadData_t;



void countNames(int *nBlocks, char* buffer,  int* histogram)
{
    char current_word[20] = "";
    int c = 0;

    for (int i=0; buffer[i]!=TERMINATOR; i++)
    {
        if(isalpha(buffer[i]) && i%CHUNKSIZE!=0 )
        {
            current_word[c++] = buffer[i];
        }else{
            current_word[c] = '\0';
            int res = getNameIndex(current_word);
            if (res != -1)
                histogram[res]++;
            c = 0;
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



void get_histogram(char *buffer, int* histogram, int num_threads)
{




}
