#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <mpi.h>
#include "helper.h"

unsigned int gol(unsigned char *grid, unsigned int dim_x, unsigned int dim_y, unsigned int time_steps)
{
	// READ ME! Parallelize this function to work with MPI. It must work even with a single processor.
	// We expect you to use MPI_Scatterv, MPI_Gatherv, and MPI_Sendrecv to achieve this.
	// MPI_Scatterv/Gatherv are checked to equal np times, and MPI_Sendrecv is expected to equal 2 * np * timesteps
	// That is, top+bottom ghost cells * all processors must execute this command * Sendrecv executed every timestep.

	unsigned char *grid_in, *grid_out, *grid_tmp;
	size_t size = sizeof(unsigned char) * dim_x * dim_y;

	grid_tmp = calloc(sizeof(unsigned char), dim_y * dim_x);
	if (grid_tmp == NULL)
		exit(EXIT_FAILURE);


	grid_in = grid;
	grid_out = grid_tmp;

	int np, rank;//,initialized;
	//MPI_Status status;
    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    //printf("Starting run with rank %i: dim_x = %i, dim_y=%i,size=%i\n",rank,dim_x,dim_y,size);
    int rcounts[np];//
    int disp[np],returndisp[np];//    
    int disp_so_far = 0;
    int length = dim_x/np*dim_y;
	for(int i=0;i<np-1;i++){
        rcounts[i]=length; //Set the size
        disp[i]=disp_so_far; //Set the displacement
        disp_so_far+=rcounts[i]; //Increment the displacement
        returndisp[i]=size-disp_so_far; //Set the displacement for the gather
        printf("i: %i, length: %i, displacement: %i, returndisp: %i,np: %i\n",i,length,disp[i],returndisp[i],np);        
    }
	rcounts[np-1]=size-((np-1)*length); //Fill the last one, making sure it completes the set
    disp[np-1] = disp_so_far;
    returndisp[np-1]=0; //Set the displacement for the gather
    //printf("np-1: %i,length: %i, disp: %i, returndisp: %i, rcounts[rank]=%i\n",np-1,size-((np-1)*length),disp[np-1],returndisp[np-1],rcounts[rank]);
	unsigned char mygrid[rcounts[rank]],my_compiled_grid[rcounts[rank]+2*dim_x],my_compiled_grid_out[rcounts[rank]+2*dim_x];
	unsigned char ghost_above[dim_x], ghost_below[dim_x];

	//Send the data
	MPI_Scatterv(grid_in,rcounts,disp,MPI_UNSIGNED_CHAR, mygrid, rcounts[rank], MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
	
    MPI_Status status,status2;

	int sender = rank; //Set the sender to this rank
	int receiver_1 = rank+1; //Set the first receiver to the next rank
    if(receiver_1>=np) {receiver_1=0;} //If the next rank is greater than the processor count cycle to 0
	int receiver_2 = rank-1; //Set the second receiver to the previous rank
	if(receiver_2<0) {receiver_2=np-1;} //If the next rank is less than zero cycle to last processor
	int sender_last_row_offset = rcounts[rank]-dim_x;
	
	printf("sender is %i, receiver 1 is %i, receiver 2 is %i\n",sender,receiver_1,receiver_2);
    printf("sender_last_row_offset is %i\n",sender_last_row_offset);
	for (int t = 0; t < time_steps; ++t)
    {
        printf("send/receive\n");
		//Send top line to ghost below
		MPI_Sendrecv(mygrid, dim_x, MPI_UNSIGNED_CHAR, receiver_1, 01,
                ghost_below, dim_x, MPI_UNSIGNED_CHAR, sender, 01,
                MPI_COMM_WORLD, &status);

        printf("send/receive 2\n");
		//Send bottom line to ghost_above
		MPI_Sendrecv(mygrid+sender_last_row_offset, dim_x, MPI_UNSIGNED_CHAR, receiver_2, 02,
                ghost_above, dim_x, MPI_UNSIGNED_CHAR, sender, 02,
                MPI_COMM_WORLD, &status2);

		
		//Compile the grid to use in evolve from received ghosts and mygrid from previous step (or from scatterv at start)
		memcpy(ghost_above,my_compiled_grid,dim_x);
		memcpy(mygrid,my_compiled_grid+dim_x,rcounts[rank]);
		memcpy(ghost_below,my_compiled_grid+dim_x+rcounts[rank],dim_x);

		for (int y = 0; y < dim_y; ++y)
		{
			for (int x = 0; x < dim_x; ++x)
            {
				evolve(my_compiled_grid,my_compiled_grid_out,dim_x,rcounts[rank]/dim_y,x,y);
				//evolve(grid_in, grid_out, dim_x, dim_y, x, y);
			}
        }
		memcpy(my_compiled_grid_out+dim_x,mygrid,rcounts[rank]); //Copy center of compiled grid out back to mygrid for sendrecv commands in the next loop
		//swap((void**)&grid_in, (void**)&grid_out);
	}

	
	//Gather the data    
    MPI_Gatherv(mygrid,rcounts[rank],MPI_UNSIGNED_CHAR, grid_out, rcounts,returndisp, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

	if (grid != grid_in)
		memcpy(grid, grid_in, size);

	free(grid_tmp);

	return cells_alive(grid, dim_x, dim_y);
}
