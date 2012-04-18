#include <dcmpi.h>

using namespace std;

char * appname = NULL;
void usage()
{
    printf("usage: %s <hostfile> <packet_size> <num_packets>\n", appname);
    exit(EXIT_FAILURE);
}


int main(int argc, char * argv[])
{
    appname = argv[0];
    if (((argc-1) != 3)) {
        usage();
    }
    int rc1 = 0;
    int rc2 = 0;

    DCLayout layout1;
    layout1.use_filter_library("libmemfilters.so");
    DCFilterInstance memWriter1("memWriter", "memWriter1");
    DCFilterInstance memReader1("memReader", "memReader1");
    layout1.add(memWriter1);
    layout1.add(memReader1);
    layout1.add_port(memWriter1, "0", memReader1, "0");
    layout1.set_exec_host_pool_file(argv[1]);
    DCBuffer workBuf1;
    workBuf1.Append(dcmpi_to_string(argv[2]) + " " +
                    dcmpi_to_string(argv[3]));
    layout1.set_init_filter_broadcast(&workBuf1);

    DCLayout layout2;
    layout2.use_filter_library("libmemfilters.so");
    DCFilterInstance memWriter2("memWriter", "memWriter2");
    DCFilterInstance memReader2("memReader", "memReader2");
    layout2.add(memWriter2);
    layout2.add(memReader2);
    layout2.add_port(memWriter2, "0", memReader2, "0");
    layout2.set_exec_host_pool_file(argv[1]);
    DCBuffer workBuf2;
    workBuf2.Append(dcmpi_to_string(argv[2]) + " " +
                    dcmpi_to_string(argv[3]));
    layout2.set_init_filter_broadcast(&workBuf2);

    layout1.execute_start();
    sleep(1);
    layout2.execute_start();
    rc1 = layout1.execute_finish();
    sleep(1);
    rc2 = layout2.execute_finish();
    return rc1 || rc2;
}
