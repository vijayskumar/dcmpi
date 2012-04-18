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
    appname = argv[0];
    if (((argc-1) != 1)) {
        usage();
    }

    DCLayout layout;
    layout.use_filter_library("libticketfilters.so");
    DCFilterInstance ticketSender("ticketSender","ticketSender");
    DCFilterInstance ticketReceiver("ticketReceiver","ticketReceiver");
    layout.add(ticketSender);
    layout.add(ticketReceiver);
    ticketReceiver.make_transparent(5);
    layout.add_port(ticketSender, "0", ticketReceiver, "0");
    layout.set_exec_host_pool_file(argv[1]);
    return layout.execute();
}
