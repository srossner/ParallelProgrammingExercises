#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "helper.h"



void reverse_str_(char *str, int strlen)
{
    //We do not repeat the reverse n times to get a better speed up in processing time

    char* begin = str, *end = str + strlen - 1;

    while (begin < end) {
        char tmp = *end;
        *end = *begin;
        *begin = tmp;

        begin++;
        end--;
    }

}





void reverse(char *str, int strlen)
{

	// parallelize this function and make sure to call reverse_str()
	// on each processor to reverse the substring.

    int np, rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &np);


    int *send_counts = malloc(sizeof(int)*np);
    int *send_displs = malloc(sizeof(int)*np);

    //int *rec_counts = malloc(sizeof(int)*np);
    int *rec_displs = malloc(sizeof(int)*np);


    // calculate send counts and displacements
    int stepSize = strlen/np;
    int maxBlokSize = stepSize + strlen%np;

    for (int i = 0; i < np; i++)
    {
        send_counts[i] = stepSize;
        send_displs[i] = stepSize*i;

        //rec_counts[i] = stepSize;
        rec_displs[i] = strlen%np + stepSize*(np-1-i);
    }

    if(strlen%np !=0)
    {
        send_counts[np-1] += strlen%np;
        rec_displs[np-1] = 0;
        //rec_counts[0] += strlen%np;
    }

    char* rec_buf = malloc(sizeof(char)*(maxBlokSize/*+1*/));

    //rec_buf[maxBlokSize] = '\0';

    /*
    // print calculated send counts and displacements for each process
    if (0 == rank) {
        for (int i = 0; i < np; i++) {
            printf("send_counts[%d] = %d\tsend_displs[%d] = %d \n", i, send_counts[i], i, send_displs[i]);
        }
    }
    */


    // divide the data among processes as described by send_counts and send_displs
    MPI_Scatterv(str, send_counts,
                 send_displs,
                 MPI_CHAR,
                 rec_buf, send_counts[rank],
                 MPI_CHAR, 0, MPI_COMM_WORLD);


    //printf("rank = %d\t data %s \n", rank, rec_buf);
    //printf("\n");


    reverse_str_(rec_buf, send_counts[rank]);


    //printf("rank = %d\t data invers %s \n", rank, rec_buf);
    //printf("\n");



    MPI_Gatherv( rec_buf, send_counts[rank], MPI_CHAR, str, send_counts, rec_displs, MPI_CHAR,
                                                               0, MPI_COMM_WORLD);
/*
    if(rank ==0)
        printf("str invers %s \n\n", str);
*/


}
