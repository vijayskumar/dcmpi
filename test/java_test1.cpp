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
    DCFilterInstance f("test.java_test1_hello", "jhf1");
    f.make_transparent(dcmpi_file_lines(argv[1]));
    layout.add(f);
    layout.set_exec_host_pool_file(argv[1]);
    int rc = layout.execute();
    return rc;
}
