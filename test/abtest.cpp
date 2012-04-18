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
    printf("usage: %s <hostfile> <num_a> <num_b> [num_packets]\n", appname);
    exit(EXIT_FAILURE);
}

int main(int argc, char * argv[])
{
    int rc = 0;

    if (((argc-1) != 3) && ((argc-1) != 4)) {
        appname = argv[0];
        usage();
    }

    DCLayout layout;
    layout.use_filter_library("libabfilters.so");
    DCFilterInstance a("a", "a");
    DCFilterInstance b("b", "b");
    layout.add(&a);
    layout.add(&b);
    layout.add_port(a, "0", b, "0");
    a.make_transparent(atoi(argv[2]));
    b.make_transparent(atoi(argv[3]));
    if ((argc-1) == 4) {
        a.set_param("num_packets",argv[4]);
    }
    layout.set_exec_host_pool_file(argv[1]);
    layout.execute();
    return rc;
}
