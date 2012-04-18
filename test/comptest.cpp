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
    printf("usage: %s <hostfile>\n",
           appname);
    exit(EXIT_FAILURE);
}

int main(int argc, char * argv[])
{
    if ((argc-1) != 1) {
        appname = argv[0];
        usage();
    }

    DCLayout layout;
    layout.use_filter_library("libcompfilters.so");
    DCFilterInstance num_feeder("num_feeder", "num_feeder1");
    DCFilterInstance averager("averager", "averager1");
    DCFilterInstance resultreader("resultreader", "resultreader1");

    std::vector<std::string> hosts = dcmpi_file_lines_to_vector(argv[1]);
    num_feeder.bind_to_host(hosts[0]);
    averager.bind_to_cluster(hosts);
    resultreader.bind_to_host(hosts[0]);
    layout.add(num_feeder);
    layout.add(averager);
    layout.add(resultreader);

    layout.add_port(num_feeder, "0", averager, "0");
    layout.add_port(averager, "0", resultreader, "0");
    layout.set_exec_host_pool_file(argv[1]);
    layout.execute();
    return 0;
}
