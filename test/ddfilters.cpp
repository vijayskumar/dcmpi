#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <queue>
#include <vector>

#include <dcmpi.h>

using namespace std;

class master : public DCFilter
{
public:
    int process() {
        int4 i;
        int4 nhosts = get_param_as_int("nhosts");
        int4 work_packets = get_param_as_int("work_packets");
        std::string hn;
        std::map<std::string, int> host_accomplished;
        std::cout << "work begins\n";
        for (i = 0; i < work_packets; i++) {
            DCBuffer * in = read("0");
            in->unpack("s", &hn);
            in->consume();
            host_accomplished[hn] += 1;
            DCBuffer out;
            out.pack("i", 1);
            write(&out, "0", hn);
        }
        for (i = 0; i < nhosts; i++) {
            DCBuffer * in = read("0");
            in->unpack("s", &hn);
            in->consume();
            DCBuffer out;
            out.pack("i", 0);
            write(&out, "0", hn);
        }
        std::map<std::string, int>::iterator it;
        for (it = host_accomplished.begin();
             it != host_accomplished.end();
             it++) {
            std::cout << it->first << ": " << it->second << endl;
        }
        return 0;
    }
};

class slave : public DCFilter
{
public:
    int process() {
        std::string myhostname = get_param("myhostname");
        int4 reps = get_param_as_int("work_consists");
        int i, x;
        while (1) {
            DCBuffer req;
            req.pack("s", myhostname.c_str());
            write(&req, "0");
            DCBuffer * in = read("0");
            int4 go;
            in->unpack("i", &go);
            in->consume();
            if (go) {
                for (i = 0; i < reps; i++) {
                    x = dcmpi_rand();
                }
            }
            else {
                break;
            }
        }
        return 0;
    }
};

dcmpi_provide2(master, slave);

