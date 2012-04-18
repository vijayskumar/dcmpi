/***************************************************************************
    $Id: mpi_emulator.cpp,v 1.2 2005/03/18 18:24:41 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include <mpi.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <malloc.h>

#include <string>
#include <iostream>

using namespace std;

#define MAX(a,b) ((a)>(b)?(a):(b))
#define STREQ(a,b) (strcmp(a,b)==0)

int rank;
int size;

void dcmpi_args_shift(int & argc, char ** argv, int frompos = 1)
{
    int i;
    for (i = frompos; i < (argc-1); i++) {
        argv[i] = argv[i+1];
    }
    argv[argc-1] = NULL;
    argc--;
}

inline unsigned long long csnum(const std::string & num)
{
    std::string num_part(num);
    int lastIdx = num.size() - 1;
    char lastChar = num[lastIdx];
    unsigned long long multiplier = 1;
    unsigned long long kb_1 = (unsigned long long)1024;
    if ((lastChar == 'k') || (lastChar == 'K')) {
        multiplier = kb_1;
    }
    else if ((lastChar == 'm') || (lastChar == 'M')) {
        multiplier = kb_1*kb_1;
    }
    else if ((lastChar == 'g') || (lastChar == 'G')) {
        multiplier = kb_1*kb_1*kb_1;
    }
    else if ((lastChar == 't') || (lastChar == 'T')) {
        multiplier = kb_1*kb_1*kb_1*kb_1;
    }
    else if ((lastChar == 'p') || (lastChar == 'P')) {
        multiplier = kb_1*kb_1*kb_1*kb_1*kb_1;
    }

    if (multiplier != 1) {
        num_part.erase(lastIdx, 1);
    }
    unsigned long long n = strtoull(num_part.c_str(), NULL, 10);
    return n * multiplier;
}

#define checkrc(rc) if ((rc) != 0) { std::cerr << "ERROR: bad return code at " << __FILE__ << ":" << __LINE__ << std::endl << std::flush; }

char * appname = NULL;
void usage()
{
    printf("usage: %s [-w] <bufsize> <reps>\n", appname);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    int rc = 0;
    unsigned long long bufsize;
    unsigned long long reps;
    unsigned long long amount;
    MPI_Request * headreq;
    MPI_Request * requests;
    char * s_buf;
    char * r_buf;
    char hdr[152];
    bool wait_mode = false;

    if ((rc = MPI_Init(&argc, &argv)) != 0) {
        fprintf(stderr, "ERROR: MPI_Init\n");
        exit(1);
    }

    if ((rc = MPI_Comm_rank(MPI_COMM_WORLD, &rank)) != 0) {
        fprintf(stderr, "ERROR: MPI_Comm_rank\n");
        exit(1);
    }
    if ((rc = MPI_Comm_size(MPI_COMM_WORLD, &size)) != 0) {
        fprintf(stderr, "ERROR: MPI_Comm_size\n");
        exit(1);
    }

    if (size != 2) {
        fprintf(stderr, "ERROR: mpi size != 2\n");
        exit(1);
    }

    appname = argv[0];

    if (((argc-1) != 2) && ((argc-1) != 3)) {
        usage();
    }

    if (STREQ(argv[1], "-w")) {
        wait_mode = true;
        dcmpi_args_shift(argc, argv);
    }
    bufsize = csnum(argv[1]);
    reps = csnum(argv[2]);
    amount = bufsize * reps;

    unsigned long long i;
    double t_start = 0.0, t_end = 0.0, t = 0.0;
    headreq = (MPI_Request*)malloc(sizeof(MPI_Request) * (reps));
    requests = (MPI_Request*)malloc(sizeof(MPI_Request) * (reps));
    s_buf = new char[bufsize];
    r_buf = new char[bufsize];

    for (i = 0; i < bufsize; i++) {
        s_buf[i] = 'a';
        r_buf[i] = 'b';
    }

    t_start = MPI_Wtime();
    if (rank == 0) {
        for (i = 0; i < reps; i++) {
            checkrc(MPI_Isend(hdr, sizeof(hdr), MPI_CHAR, 1, 101,
                              MPI_COMM_WORLD, &headreq[i]));
            checkrc(MPI_Isend(s_buf, bufsize, MPI_CHAR, 1, 101,
                              MPI_COMM_WORLD, &requests[i]));
        }
        for (i = 0; i < reps; i++) {
            checkrc(MPI_Wait(&headreq[i], MPI_STATUS_IGNORE));
            checkrc(MPI_Wait(&requests[i], MPI_STATUS_IGNORE));
        }
        checkrc(MPI_Recv(r_buf, 4, MPI_CHAR, 1, 101, MPI_COMM_WORLD, MPI_STATUS_IGNORE));
    }
    else {
        for (i = 0; i < reps; i++) {
            checkrc(MPI_Irecv(hdr, sizeof(hdr), MPI_CHAR, 0, 101,
                              MPI_COMM_WORLD, &headreq[0]));
            checkrc(MPI_Wait(&headreq[0], MPI_STATUS_IGNORE));
            delete[] r_buf;
            r_buf = new char[bufsize];
            checkrc(MPI_Recv(r_buf, bufsize, MPI_CHAR, 0, 101, MPI_COMM_WORLD, MPI_STATUS_IGNORE));
        }
        checkrc(MPI_Send(s_buf, 4, MPI_CHAR, 0, 101, MPI_COMM_WORLD));
//         sleep(2);
//         checkrc(MPI_Irecv(hdr, sizeof(hdr), MPI_CHAR, 0, 101,
//                           MPI_COMM_WORLD, &headreq[0]));
//         checkrc(MPI_Cancel(&headreq[0]));
    }
    t_end = MPI_Wtime();
    t = t_end - t_start;
    if (rank == 0) {
        printf("MB/sec %.2f bytes %llu bufsize %llu reps %llu seconds %f\n",
               ((amount*1.0)/t) / (1000*1000), amount, bufsize, reps, t);
    }

    if ((rc = MPI_Finalize()) != 0) {
        fprintf(stderr, "ERROR: MPI_Finalize\n");
        exit(1);
    }
    
    return 0;
}
