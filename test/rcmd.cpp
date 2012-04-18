#include <dcmpi.h>

using namespace std;

char * appname = NULL;
void usage()
{
    printf("usage: %s <hostfile> <command> [command args]...\n", appname);
    exit(EXIT_FAILURE);
}

int main(int argc, char * argv[])
{
    if ((argc-1) < 2) {
        appname = argv[0];
        usage();
    }

    std::string rcmd;
    DCLayout layout;
    layout.use_filter_library("librcmdfilters.so");
    DCFilterInstance rcmd_filter("rcmd_filter","rcmd_filter");
    int num_filters;
    int i;
    layout.add(rcmd_filter);

    for (i = 2; i < argc; i++) {
        rcmd += " ";
        rcmd += argv[i];
    }
    dcmpi_string_trim(rcmd);
    rcmd_filter.set_param("rcmd",rcmd);

    std::vector<std::string> lines = dcmpi_file_lines_to_vector(argv[1]);
    num_filters = lines.size();
    for (i = 0; i < num_filters; i++) {
        rcmd_filter.bind_to_host(lines[i]);
    }

    return layout.execute();
}

