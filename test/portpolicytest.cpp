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
    layout.use_filter_library("libportpolicyfilters.so");
    DCFilterInstance sender("policy_sender",  "sender");
    DCFilterInstance slow_guy("policy_slow_guy", "slow_guy");
    DCFilterInstance fast_guy("policy_fast_guy", "fast_guy");
    layout.add(&sender);
    layout.add(&slow_guy);
    layout.add(&fast_guy); 
    layout.add_port(sender, "0", slow_guy, "0");
    layout.add_port(sender, "0", fast_guy, "0");
    layout.set_exec_host_pool_file(argv[1]);
    layout.execute();
    return rc;
}


using namespace std;
