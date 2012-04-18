/***************************************************************************
    $Id: consolebcast.cpp,v 1.1 2006/05/26 22:06:53 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include "dcmpi.h"

using namespace std;

char * appname = NULL;
void usage()
{
    printf("usage: %s <hostfile> <packet_size> <npackets>\n", appname);
    exit(EXIT_FAILURE);
}

int main(int argc, char * argv[])
{
    if ((argc-1) != 3) {
        appname = argv[0];
        usage();
    }

    DCLayout layout;
    DCFilterInstance console("<console>", "console1");
    layout.use_filter_library("libconsolefilters.so");
    layout.add(console);
    std::vector<std::string> hosts = dcmpi_file_lines_to_vector(argv[1]);
    uint u;
    for (u = 0; u < hosts.size(); u++) {
        DCFilterInstance * f = new DCFilterInstance("console_receiver",
                                                    dcmpi_to_string("recv")+
                                                    dcmpi_to_string(u));
        layout.add(f);
        layout.add_port(&console, "0", f, "0");
        f->bind_to_host(hosts[u]);
    }
    int packet_size = dcmpi_csnum(argv[2]);
    int npackets = dcmpi_csnum(argv[3]);
    DCFilter * consoleFilter = layout.execute_start();
    DCBuffer * bufout = new DCBuffer(packet_size);
    memset(bufout->getPtr(), 0, packet_size);
    for (int i = 0; i < npackets; i++) {
        bufout->setUsedSize(packet_size);
        consoleFilter->write_broadcast(bufout, "0");
    }
    delete bufout;
    return layout.execute_finish();
}
