#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <mpi.h>
#include "helper.h"


void print_pice(unsigned char *grid, unsigned int dim_x, unsigned int dim_y)
{
    unsigned char (*c_grid)[dim_x] = (unsigned char (*)[dim_x])grid;

    size_t size = sizeof(unsigned char) * dim_x + 4;

    unsigned char *row = malloc(size);
    if (row == NULL)
        exit(EXIT_FAILURE);

    memset(row, '-', size);
    row[0] = '+';
    row[size - 3] = '+';
    row[size - 2] = '\n';
    row[size - 1] = '\0';

    printf("%s", row);

    row[0] = '|';
    row[size - 3] = '|';

    for (int i = 0; i < dim_y; ++i) {
        for (int j = 0; j < dim_x; ++j) {
            if (c_grid[i][j] == 0)
                row[j + 1] = ' ';
            else
                row[j + 1] = '*';
        }
        printf("%s", row);
    }

    memset(row, '-', size);
    row[0] = '+';
    row[size - 3] = '+';
    row[size - 2] = '\n';
    row[size - 1] = '\0';

    printf("%s", row);

    free(row);
}


int DEBUG = 0;


unsigned int gol(unsigned char *grid, unsigned int dim_x, unsigned int dim_y, unsigned int time_steps)
{
    //MPI
    int np, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if(np == 1)
    {
        unsigned char *grid_in, *grid_out, *grid_tmp;
        size_t size = sizeof(unsigned char) * dim_x * dim_y;

        grid_tmp = calloc(sizeof(unsigned char), dim_y * dim_x);
        if (grid_tmp == NULL)
            exit(EXIT_FAILURE);


        grid_in = grid;
        grid_out = grid_tmp;

        for (int t = 0; t < time_steps; ++t)
        {
            for (int y = 0; y < dim_y; ++y)
            {
                for (int x = 0; x < dim_x; ++x)
                {
                    evolve(grid_in, grid_out, dim_x, dim_y, x, y);
                }
            }
            swap((void**)&grid_in, (void**)&grid_out);
        }

        if (grid != grid_in)
            memcpy(grid, grid_in, size);

        free(grid_tmp);

        return cells_alive(grid, dim_x, dim_y);
    }

    unsigned char *grid_in, *grid_out;
    size_t size = sizeof(unsigned char) * dim_x * dim_y;

    grid_in = grid;



    //number of elements to send

    int number_of_rows = dim_y/np;
    int number_of_rows_rest = dim_y%np;

    int number_of_row_tims_dim_x        = ((int)(number_of_rows)) * dim_x;
    int number_of_row_tims_dim_x_start  = number_of_row_tims_dim_x + dim_x;
    int number_of_row_tims_dim_x_middel = number_of_row_tims_dim_x_start + dim_x;
    int number_of_row_tims_dim_x_end    = number_of_row_tims_dim_x_start + (number_of_rows_rest) * dim_x;


    //elements for scatter count
    int *scatter_send_displacment = calloc(sizeof(int),np);
    int *scatter_send_counts= calloc(sizeof(int),np);

    // elements for gather
    int *gather_rec_displacment = calloc(sizeof(int),np);
    int *gather_rec_counts = calloc(sizeof(int),np);

    scatter_send_displacment[0]= 0;
    scatter_send_counts[0] = number_of_row_tims_dim_x_start ;

    gather_rec_displacment[0] = 0;
    gather_rec_counts[0] = number_of_row_tims_dim_x;

    for(int i = 1; i < (np-1) ; i++)
    {
        scatter_send_displacment[i]= number_of_row_tims_dim_x - dim_x + (i-1) * (number_of_row_tims_dim_x);
        scatter_send_counts[i] = number_of_row_tims_dim_x_middel;

        gather_rec_displacment[i] = number_of_row_tims_dim_x * i;
        gather_rec_counts[i] = number_of_row_tims_dim_x;
    }

    scatter_send_displacment[np-1]= number_of_row_tims_dim_x - dim_x + (np-2) * (number_of_row_tims_dim_x);
    scatter_send_counts[np-1] = number_of_row_tims_dim_x_end;

    gather_rec_displacment[np-1] = number_of_row_tims_dim_x * (np-1);
    gather_rec_counts[np-1] = number_of_row_tims_dim_x_end - dim_x;

    unsigned char *scatter_rec_buffer = calloc(sizeof(unsigned char), (number_of_row_tims_dim_x_end + (2*dim_x)));

    grid_out = calloc(sizeof(unsigned char), (number_of_row_tims_dim_x_end + dim_x));

    if (grid_out == NULL)
    {
        printf("EXIT_FAILURE %i \n\n", rank);
        exit(EXIT_FAILURE);
    }
    MPI_Barrier( MPI_COMM_WORLD );


    MPI_Scatterv(grid_in, scatter_send_counts,
                 scatter_send_displacment,
                 MPI_UNSIGNED_CHAR,
                 scatter_rec_buffer, scatter_send_counts[rank],
                 MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);


    if(DEBUG)
    {
        MPI_Barrier( MPI_COMM_WORLD );

        printf("rank %i START \n", rank);
        print_pice(scatter_rec_buffer, dim_x, scatter_send_counts[rank]/dim_x);
        printf("rank %i ENDE\n\n\n", rank);

        MPI_Barrier( MPI_COMM_WORLD );
    }




    int end_y =  number_of_rows;

    if(rank == (np-1))
    {
        end_y += number_of_rows_rest;
    }



    MPI_Status status;

    unsigned char* sendrecv_send_buffer;
    unsigned char* sendrecv_rec_buffer;

    int sendrecv_send_count, sendrecv_rec_count;
    int sendrecv_sender, sendrecv_recver;


    for (int t = 0; t < time_steps; ++t)
    {


        for (int y = 0; y < end_y; ++y)
        {
            for (int x = 0; x < dim_x; ++x)
            {

                evolve(scatter_rec_buffer, grid_out, dim_x, dim_y, x, y);

            }
        }

        swap((void**)&scatter_rec_buffer, (void**)&grid_out);

        //send Down
        if(rank == 0)
            sendrecv_send_buffer = &scatter_rec_buffer[number_of_row_tims_dim_x - dim_x];
        else
            sendrecv_send_buffer = &scatter_rec_buffer[number_of_row_tims_dim_x];


        sendrecv_rec_buffer = scatter_rec_buffer;

        sendrecv_send_count = dim_x;
        sendrecv_rec_count = dim_x;
        sendrecv_sender = rank;
        sendrecv_recver = rank+1;


        if(rank == (np-1)) // send down
        {
            sendrecv_send_count = 0;
            sendrecv_recver = 0;
        }

        if(rank == 0)
        {
            sendrecv_rec_count = 0;
        }


        MPI_Barrier( MPI_COMM_WORLD );

        printf("DOWN rank %i sendrecv sender: %i recerver: %i\n", rank, sendrecv_sender, sendrecv_recver );
        printf("DOWN rank %i sendrecv sendrecv_send_count: %i sendrecv_rec_count: %i\n\n", rank, sendrecv_send_count, sendrecv_rec_count );


        MPI_Sendrecv( // send down
            sendrecv_send_buffer,   //*sendbuf,
            sendrecv_send_count,    //int sendcount,
            MPI_UNSIGNED_CHAR,      //MPI_Datatype sendtype,
            sendrecv_recver,        //int dest,
            0,                      //int sendtag,
            sendrecv_rec_buffer,    //void *recvbuf,
            sendrecv_rec_count,     //int recvcount,
            MPI_UNSIGNED_CHAR,      //MPI_Datatype recvtype,
            sendrecv_sender,        //int source,
            0,                      //int recvtag,
            MPI_COMM_WORLD,         //MPI_Comm comm,
            &status                 //MPI_Status *status
        );



        printf("DOWN FINISHED rank %i sendrecv \n \n", rank);


        MPI_Barrier( MPI_COMM_WORLD );



        sendrecv_send_buffer = scatter_rec_buffer;

        if(rank == 0)
            sendrecv_rec_buffer = &scatter_rec_buffer[number_of_row_tims_dim_x - dim_x];
        else
            sendrecv_rec_buffer = &scatter_rec_buffer[number_of_row_tims_dim_x];

        sendrecv_send_count = dim_x;
        sendrecv_rec_count = dim_x;
        sendrecv_sender = rank;
        sendrecv_recver = rank-1;


        if(rank == (np-1)) // send up
        {
            sendrecv_rec_count = 0;
            sendrecv_recver = 0;
        }

        if(rank == 0)
        {
            sendrecv_send_count = 0;
            sendrecv_sender = np-1;
        }


        printf("UP rank %i sendrecv \n \n", rank);
        MPI_Sendrecv( // up
            sendrecv_send_buffer,   //*sendbuf,
            sendrecv_send_count,    //int sendcount,
            MPI_UNSIGNED_CHAR,      //MPI_Datatype sendtype,
            sendrecv_recver,        //int dest,
            0,                      //int sendtag,
            sendrecv_rec_buffer,    //void *recvbuf,
            sendrecv_rec_count,     //int recvcount,
            MPI_UNSIGNED_CHAR,      //MPI_Datatype recvtype,
            sendrecv_sender,        //int source,
            0,                      //int recvtag,
            MPI_COMM_WORLD,         //MPI_Comm comm,
            &status                 //MPI_Status *status
        );



        printf("End UP rank %i sendrecv \n \n", rank);

        MPI_Barrier( MPI_COMM_WORLD );


    }



    if(DEBUG)
    {
        MPI_Barrier( MPI_COMM_WORLD );

        printf("rank %i START \n", rank);
        print_pice(scatter_rec_buffer, dim_x, scatter_send_counts[rank]/dim_x);
        printf("rank %i ENDE\n\n\n", rank);

        MPI_Barrier( MPI_COMM_WORLD );
    }

    unsigned char *gatherv_rec_buffer;

    // move from gost rows
    if(rank != 0 )
        gatherv_rec_buffer = &scatter_rec_buffer[ dim_x ];
    else
        gatherv_rec_buffer = scatter_rec_buffer;

    MPI_Barrier( MPI_COMM_WORLD );

    MPI_Gatherv( gatherv_rec_buffer, gather_rec_counts[rank],
                 MPI_UNSIGNED_CHAR,
                 grid_in, scatter_send_counts,
                 gather_rec_displacment,
                 MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    if (grid != grid_in)
        memcpy(grid, grid_in, size);


    if(DEBUG)printf("rank %i free grid_out  \n", rank);
    free(grid_out);

    MPI_Barrier( MPI_COMM_WORLD );

    if(rank == 0 && DEBUG)
    {
        printf("RESULT \n");
        print_pice(grid_in, dim_x, dim_y);
        printf("ENDE \n\n\n");
    }


    MPI_Barrier( MPI_COMM_WORLD );

    free(scatter_rec_buffer);

    if(DEBUG)
    {
        MPI_Barrier( MPI_COMM_WORLD );
        printf("rank %i ENDE OF ALL  \n\n\n", rank);
        MPI_Barrier( MPI_COMM_WORLD );
    }



    return cells_alive(grid, dim_x, dim_y);
}
