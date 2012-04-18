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
    printf("usage: %s [-wp <INT>] [-wc <INT>] <hostfile>\n", appname);
    printf("\n");
    printf("    -wp gives how many work packets up for grabs (default 1000)\n");
    printf("    -wc gives how much work done per packet (default 100000)\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char * argv[])
{
    int work_packets = 1000;
    int work_consists = 100000;
    while (argc > 1) {
        if (!strcmp(argv[1], "-wp")) {
            work_packets = atoi(argv[2]);
            dcmpi_args_shift(argc, argv);
        }
        else if (!strcmp(argv[1], "-wc")) {
            work_consists = atoi(argv[2]);
            dcmpi_args_shift(argc, argv);            
        }
        else {
            break;
        }
        dcmpi_args_shift(argc, argv);
    }
    if ((argc-1) != 1) {
        appname = argv[0];
        usage();
    }
    std::vector<std::string> hosts = dcmpi_file_lines_to_vector(argv[1]);
    DCLayout layout;
    layout.use_filter_library("libddfilters.so");
    DCFilterInstance master("master", "master");
    master.bind_to_host(hosts[0]);
    layout.add(master);
    layout.set_param_all("nhosts", dcmpi_to_string(hosts.size()));
    layout.set_param_all("work_packets", dcmpi_to_string(work_packets));
    layout.set_param_all("work_consists", dcmpi_to_string(work_consists));
    uint u;
    for (u = 0; u < hosts.size(); u++) {
        DCFilterInstance * slave = new DCFilterInstance(
            "slave", dcmpi_to_string("slave_") + dcmpi_to_string(u));
        slave->bind_to_host(hosts[u]);
        slave->add_label(hosts[u]);
        slave->set_param("myhostname", hosts[u]);
        layout.add_port(&master, "0", slave, "0");
        layout.add_port(slave, "0", &master, "0");
        layout.add(slave);
    }
    return layout.execute();
}
