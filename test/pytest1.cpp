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
    layout.use_filter_library("pyfilters.py");
    DCFilterInstance A("A", "A1");
    DCFilterInstance Z("Z", "Z1");
    layout.add(A);
    layout.add(Z);
    layout.add_port(A, "0", Z, "0");
    layout.set_exec_host_pool_file(argv[1]);
    return layout.execute();
}
