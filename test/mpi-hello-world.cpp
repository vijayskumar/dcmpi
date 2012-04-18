#include <mpi.h> 

#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

using namespace std;

int main(int argc, char *argv[])
{
    int myrank, size;

    MPI_Init(&argc, &argv);                 /* Initialize MPI       */

    MPI_Comm_rank(MPI_COMM_WORLD, &myrank); /* Get my rank          */
    MPI_Comm_size(MPI_COMM_WORLD, &size);   /* Get the total 
                                               number of processors */
    char localhost[1024];
    gethostname(localhost, sizeof(localhost));
    localhost[sizeof(localhost)-1] = 0;
    printf("Processor %d of %d: Hello World on %s\n", 
           myrank+1, size, localhost);
    fflush(stdout);

    MPI_Finalize();                         /* Terminate MPI        */

    return 0;
}
