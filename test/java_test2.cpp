#include <dcmpi.h>

using namespace std;

char * appname = NULL;
void usage()
{
    printf("usage: %s <host_file>\n", appname);
    exit(EXIT_FAILURE);
}

int main(int argc, char * argv[])
{
    DCLayout layout;

    if ((argc-1) != 1) {
        appname = argv[0];
        usage();
    }

    layout.use_filter_library("java_test_filters.jar");
    DCFilterInstance f1("test.java_test2_sender", "jsender");
    DCFilterInstance f2("test.java_test2_receiver", "jreceiver");
    layout.add(f1);
    layout.add(f2);
    layout.add_port(f1, "0", f2, "0");
    layout.set_exec_host_pool_file(argv[1]);
    int rc = layout.execute();
    return rc;
}
