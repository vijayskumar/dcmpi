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
    printf("usage: %s <hostfile>\n", appname);
    exit(EXIT_FAILURE);
}


int main(int argc, char * argv[])
{
    if ((argc-1) != 1) {
        appname = argv[0];
        usage();
    }

    DCLayout layout;
    layout.use_filter_library("java_test_filters.jar");
    layout.use_filter_library("libjava_test4_filter_cxx.so");
    int lines = dcmpi_file_lines(argv[1]);
    layout.set_exec_host_pool_file(argv[1]);
    std::vector< DCFilterInstance*> filters;
    int i;
    for (i = 0; i < lines; i++) {
        std::string filter_name;
        if (dcmpi_rand()%2 == 0) {
            filter_name = "test.java_test4_filter_java";
        }
        else {
            filter_name = "java_test4_filter_cxx";
        }
        filters.push_back(
            new DCFilterInstance(filter_name, "f" + dcmpi_to_string(i)));
        layout.add(filters[i]);
        if (i) {
            layout.add_port(filters[i-1], "0",
                            filters[i], "0");
            if (i == lines-1) {
                filters[i]->set_param("mode", "inonly");
            }
            else {
                filters[i]->set_param("mode", "inout");
            }
            
        }
        else {
            filters[i]->set_param("mode", "outonly");
        }
    }
    layout.set_param_all("numfilters", dcmpi_to_string(lines));
    return layout.execute();
}
