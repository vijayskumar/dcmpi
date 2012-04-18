#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <queue>
#include <vector>

#include <dcmpi.h>

using namespace std;

char * appname = NULL;
void usage()
{
    printf("usage: %s [-n] <hostfile>\n", appname);
    printf("\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char * argv[])
{
    bool compress = true;
    if (argc > 1 && !strcmp(argv[1], "-n")) {
        compress = false;
        dcmpi_args_shift(argc,argv);
    }
    if ((argc-1) != 1) {
        appname = argv[0];
        usage();
    }
    DCLayout layout;
    layout.set_param_all("compress", compress?"1":"0");
    layout.set_exec_host_pool_file(argv[1]);
    layout.use_filter_library("libcomprfilters.so");
    DCFilterInstance producer("producer", "producer");
    DCFilterInstance consumer("consumer", "consumer");
    layout.add(producer);
    layout.add(consumer);
    layout.add_port(producer, "0", consumer, "0");
    return layout.execute();
}

