#include <mpi.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>
#include <vector>

#include <unistd.h>

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define checkrc(rc) if ((rc) != 0) { std::cerr << "ERROR: bad return code at " << __FILE__ << ":" << __LINE__ << std::endl << std::flush; }


int rank;
int size;

char * appname = NULL;
void usage()
{
    printf("usage: %s [-b] [-mtu <MTU>] <bufsize> <reps>>\n", appname);
    printf("\n-b is for blocking\n");
    exit(EXIT_FAILURE);
}

void args_shift(int * argc, char ** argv)
{
    int i;
    for (i = 1; i < ((*argc)-1); i++) {
        argv[i] = argv[i+1];
    }
    argv[(*argc)-1] = NULL;
    *argc = *argc - 1;
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

int main(int argc, char *argv[])
{
    int rc = 0;
    unsigned long long bufsize;
    unsigned long long reps;
    unsigned long long amount;
    int blocking = 0;
    std::vector<MPI_Request> requests;
    char * s_buf;
    char * r_buf;
    unsigned long long mtu = ~((unsigned long long)0);

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

    /* snag optional args */
    while (argc > 1) {
        if (strcmp(argv[1], "-b") == 0) {
            blocking = 1;
            args_shift(&argc, argv);
        }
        else if (strcmp(argv[1], "-mtu") == 0) {
            mtu = csnum(argv[2]);
            args_shift(&argc, argv);
            args_shift(&argc, argv);
        }
        else {
            break;
        }
    }

    if (argc != 3) {
        usage();
    }

    bufsize = csnum(argv[1]);
    reps = csnum(argv[2]);
    amount = bufsize * reps;

    unsigned long long i;
    double t_start = 0.0, t_end = 0.0, t = 0.0;
    s_buf = (char*)malloc(bufsize);
    r_buf = (char*)malloc(bufsize);

    for (i = 0; i < bufsize; i++) {
        s_buf[i] = 'a';
        r_buf[i] = 'b';
    }

    t_start = MPI_Wtime();
    if (!blocking) {
        if (rank == 0) {
            for (i = 0; i < reps; i++) {
                char * offset = s_buf;
                char * end = s_buf + bufsize;
                while (offset < (s_buf + bufsize)) {
                    int trans_size = MIN(mtu, bufsize);
                    trans_size = MIN(trans_size, end - offset);
                    MPI_Request req;
                    checkrc(MPI_Isend(offset, trans_size, MPI_CHAR, 1, 101,
                                      MPI_COMM_WORLD, &req));
                    requests.push_back(req);
                    offset += trans_size;
                }
            }
            checkrc(MPI_Waitall(requests.size(),
                                &requests[0], MPI_STATUSES_IGNORE));
            MPI_Recv(r_buf, 4, MPI_CHAR, 1, 101, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
        }
        else {
            for (i = 0; i < reps; i++) {
                char * offset = r_buf;
                char * end = r_buf + bufsize;
                while (offset < (r_buf + bufsize)) {
                    int trans_size = MIN(mtu, bufsize);
                    trans_size = MIN(trans_size, end - offset);
                    MPI_Request req;
                    checkrc(MPI_Irecv(offset, trans_size, MPI_CHAR, 0, 101,
                                      MPI_COMM_WORLD, &req));
                    requests.push_back(req);
                    offset += trans_size;
                }
            }
            checkrc(MPI_Waitall(requests.size(), &requests[0],
                                MPI_STATUSES_IGNORE));
            MPI_Send(s_buf, 4, MPI_CHAR, 0, 101, MPI_COMM_WORLD);
        }
    }
    else {
        if (rank == 0) {
            for (i = 0; i < reps; i++) {
                rc = MPI_Send(s_buf, bufsize, MPI_CHAR, 1, 101, MPI_COMM_WORLD);
                if (rc != 0) {
                    fprintf(stderr, "ERROR: MPI_Send\n");
                    exit(1);
                }
            }
            MPI_Recv(r_buf, 4, MPI_CHAR, 1, 101, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
        }
        else {
            for (i = 0; i < reps; i++) {
                rc = MPI_Recv(r_buf, bufsize, MPI_CHAR, 0, 101,
                              MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
                if (rc != 0) {
                    fprintf(stderr, "ERROR: MPI_Irecv\n");
                    exit(1);
                }
            }
            MPI_Send(s_buf, 4, MPI_CHAR, 0, 101, MPI_COMM_WORLD);
        }
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
