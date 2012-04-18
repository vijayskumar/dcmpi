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

    DCLayout layout;
    layout.use_filter_library("libmemfilters.so");
    DCFilterInstance memWriter("memWriter", "memWriter");
    DCFilterInstance memReader("memReader", "memReader");
    layout.add(memWriter);
    layout.add(memReader);
    layout.add_port(memWriter, "memOut", memReader, "memIn");
    layout.set_exec_host_pool_file(argv[1]);
    DCBuffer workBuf;
    workBuf.Append(dcmpi_to_string(argv[2]) + " " + dcmpi_to_string(argv[3]));
    layout.set_init_filter_broadcast(&workBuf);
    return layout.execute();
}
