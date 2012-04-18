#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "dcmpi.h"

char * appname = NULL;
void usage()
{
    printf("usage: %s <hostfile>\n", appname);
    exit(EXIT_FAILURE);
}

int main(int argc, char * argv[])
{
    int rc = 0;

    if ((argc-1) != 1) {
        appname = argv[0];
        usage();
    }

    DCLayout layout;
    layout.use_filter_library("liboverwritefilters.so");
    DCFilterInstance a("overwrite1", "overwrite1");
    DCFilterInstance b("overwrite2", "overwrite2");
    layout.add(&a);
    layout.add(&b);
    layout.add_port(a, "0", b, "0");
    layout.set_exec_host_pool_file(argv[1]);
    layout.execute();
    return rc;
}
