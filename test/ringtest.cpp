/***************************************************************************
    $Id: ringtest.cpp,v 1.4 2005/06/02 21:00:04 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

using namespace std;

#include <dcmpi.h>

using namespace std;

char * appname = NULL;
void usage()
{
    printf("usage: %s <hostfile> <num_filters>\n", appname);
    exit(EXIT_FAILURE);
}

int main(int argc, char * argv[])
{
    if ((argc-1) != 2) {
        appname = argv[0];
        usage();
    }

    int i;
    int num_filters = atoi(argv[2]);
    if (num_filters < 1) {
        usage();
    }
    DCLayout layout;
    layout.use_filter_library("libringfilters.so");
    DCFilterInstance console("<console>", "console1");
    layout.add(console);
    std::vector<DCFilterInstance*> filters;

    for (i = 0; i < num_filters; i++) {
        DCFilterInstance * ring = new DCFilterInstance("ringfilter",
                                                       "f" +
                                                       dcmpi_to_string(i));
        layout.add(ring);
        filters.push_back(ring);
        if (i == 0) {
            layout.add_port(&console, "0", filters[0], "0");
        }
        else {
            layout.add_port(filters[i-1], "0", filters[i], "0");
        }
        ring->set_param("myid", dcmpi_to_string(i));
    }
    layout.add_port(filters[num_filters-1], "0", &console, "0");
    layout.set_exec_host_pool_file(argv[1]);
    
    DCFilter * consoleFilter = layout.execute_start();
    DCBuffer bufout;
    bufout.Append("console was here\n");
    consoleFilter->write(&bufout, "0");
    DCBuffer * bufin = consoleFilter->read("0");
    std::string s;
    bufin->Extract(&s);
    std::string expected = "console was here\n";
    for (i = 0; i < num_filters; i++) {
        expected += "f" + dcmpi_to_string(i) + " was here\n";
    }
    cout << "console: received back " << s;
        
    layout.execute_finish();
    for (i = 0; i < num_filters; i++) {
        delete filters[i];
    }
    if (s != expected) {
        std::cerr << "ERROR: s != expected: "
                  << s << "\n!=\n" << expected
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        return 1;
    }
    return 0;
}
