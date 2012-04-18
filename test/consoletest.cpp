/***************************************************************************
    $Id: consoletest.cpp,v 1.4 2005/06/02 21:00:04 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include "dcmpi.h"

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
    layout.use_filter_library("libconsolefilters.so");
    DCFilterInstance console("<console>", "console1");
    DCFilterInstance ca("ca", "ca1");
    DCFilterInstance cb("cb", "cb1");

    layout.add_port(ca, "0", cb, "0");
    layout.add_port(cb, "0", console, "0");
    layout.add_port(console, "0", ca, "0");

    layout.add(console);
    layout.add(ca);
    layout.add(cb);
    layout.set_exec_host_pool_file(argv[1]);
    
    DCFilter * consoleFilter = layout.execute_start();
    for (int i = 0; i < 10; i++) {
        DCBuffer bufout;
        bufout.Append("hello " + dcmpi_to_string(i) + " from console");
        consoleFilter->write(&bufout, "0");
    }

    for (int i = 0; i < 10; i++) {
        DCBuffer * bufin;
        bufin = consoleFilter->read("0");
        std::string s;
        bufin->Extract(&s);
        cout << "console: received " << s << endl;
    }
    layout.execute_finish();
}
