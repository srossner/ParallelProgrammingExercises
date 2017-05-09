#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "histogram.h"

void get_histogram(int nBlocks, block_t *blocks, histogram_t histogram, int num_threads)
{
    char current_word[20] = "";
    int c = 0;
    uint max_name_length = 0;

    for(int k = 0; k < NNAMES; k++)	{
        if(max_name_length < strlen(names[k]))
            max_name_length = strlen(names[k]);
    }




    for (int i = 0; i < nBlocks; i++) {

        // loop over every character
        for (uint j = 0; j < max_name_length; j++) {

            // if the character is an alphabet store it to form a word.
            if(isalpha(blocks[i][j])){
                current_word[c++] = blocks[i][j];
            }

            // if the character is not an alphabet it is the end of a word.
            // Compare the word with the list of names.
            else {
                current_word[c] = '\0';
                for(int k = 0; k < NNAMES; k++)	{
                    if(!strcmp(current_word, names[k]))
                        histogram[k]++;
                }
                c = 0;
            }
        }
    }
}
