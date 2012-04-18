#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include <dcmpi.h>

char * appname = NULL;
void usage()
{
    printf("usage: %s [-p] <hostfile> <packet_size> <num_packets>\n", appname);
    exit(EXIT_FAILURE);
}

int main(int argc, char * argv[])
{
    int rc = 0;
    bool prealloc = false;

    appname = argv[0];
    if (((argc-1) != 3) && ((argc-1) != 4)) {
        usage();
    }

    if ((argc-1) == 4) {
        if (strcmp(argv[1], "-p") == 0) {
            prealloc = true;
            dcmpi_args_shift(argc, argv);
        }
        else {
            usage();
        }
    }

    // here's what happens.
    // 1 sends large data to 2
    // 4 sends large data to 3
    // 2 acks 1 when it's done receiving
    // 3 acks 1 when it's done receiving from 4
    // 1 reports MB/sec

    DCLayout layout;
    layout.use_filter_library("libbidibwfilters.so");
    DCFilterInstance bidibw1("bidibwsender",   "bidibw1");
    DCFilterInstance bidibw2("bidibwreceiver", "bidibw2");
    DCFilterInstance bidibw3("bidibwreceiver", "bidibw3");
    DCFilterInstance bidibw4("bidibwsender",   "bidibw4");
    bidibw1.set_param("collectsFinalResults", "true");
    bidibw4.set_param("collectsFinalResults", "false");
    layout.add(bidibw1);
    layout.add(bidibw2);
    layout.add(bidibw3);
    layout.add(bidibw4);
    layout.add_port(bidibw1, "data", bidibw2, "data");
    layout.add_port(bidibw2, "ack", bidibw1, "ack");
    layout.add_port(bidibw4, "data", bidibw3, "data");
    layout.add_port(bidibw3, "ack", bidibw1, "ack");

    layout.set_param_all("prealloc", prealloc?"true":"false");
    layout.set_param_all("packet_size", argv[2]);
    layout.set_param_all("num_packets", argv[3]);
    
    layout.set_exec_host_pool_file(argv[1]);
    if (dcmpi_file_lines(argv[1]) != 2) {
        std::cerr << "ERROR: need to pass host file with 2 hosts in it\n";
        exit(1);
    }
    layout.execute();
    return rc;
}
