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
    printf("usage: %s [-py] <hostfile> <packet_size> <num_packets>\n", appname);
    exit(EXIT_FAILURE);
}

int main(int argc, char * argv[])
{
    int rc = 0;
    bool python_mode = false;
    appname = argv[0];
    while (argc-1 > 3) {
        if (!strcmp(argv[1], "-py")) {
            python_mode = true;
            dcmpi_args_shift(argc, argv);
        }
        else {
            std::cerr << "ERROR: invalid parameter " << argv[1]
                      << std::endl << std::flush;
        }
    }
    if ((argc-1) != 3) {
        usage();
    }

    DCLayout layout;
    if (python_mode) {
        layout.use_filter_library("pyfilters.py");        
    }
    else {
        layout.use_filter_library("libbwfilters.so");
    }

    layout.set_param_all("packet_size", dcmpi_to_string(dcmpi_csnum(argv[2])));
    layout.set_param_all("num_packets", dcmpi_to_string(dcmpi_csnum(argv[3])));
    DCFilterInstance bw1("bw1", "bw1");
    DCFilterInstance bw2("bw2", "bw2");
    layout.add(&bw1);
    layout.add(&bw2);
    layout.add_port(&bw1, "0", &bw2, "0");
    layout.add_port(&bw2, "1", &bw1, "1");
    layout.set_exec_host_pool_file(argv[1]);
    layout.execute();
    return rc;
}
