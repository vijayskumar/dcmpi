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

    if (((argc-1) != 1)) {
        appname = argv[0];
        usage();
    }

    DCLayout layout;
    layout.use_filter_library("libmemfilters.so");
    DCFilterInstance memWriter("memWriter", "memWriter");
    DCFilterInstance memReader("memReader", "memReader");
    layout.add(&memWriter);
    layout.add(&memReader);
    layout.add_port(memWriter, "0", memReader, "0");
    DCBuffer workBuf;
    workBuf.Append("0 10");
    layout.set_init_filter_broadcast(&workBuf);
    layout.set_exec_host_pool_file(argv[1]);
    layout.execute();
    return rc;
}
