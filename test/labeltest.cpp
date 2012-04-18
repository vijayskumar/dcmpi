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
    layout.use_filter_library("liblabelfilters.so");
    DCFilterInstance labelSender("labelSender","labelSender");
    layout.add(labelSender);
    for (int i = 0; i < 5; i++) {
        DCFilterInstance * labelReceiver = new DCFilterInstance("labelReceiver","labelReceiver_" + dcmpi_to_string(i));
        layout.add(labelReceiver);
        labelReceiver->add_label(dcmpi_to_string("foo") +
                                 dcmpi_to_string(i));
        layout.add_port(&labelSender, "0", labelReceiver, "0");
    }
    layout.set_exec_host_pool_file(argv[1]);
    return layout.execute();
}
