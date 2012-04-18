/***************************************************************************
    $Id: bwfilters.cpp,v 1.5 2005/12/12 14:49:43 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include <dcmpi.h>

#include <iomanip>

using namespace std;

class bw1 : public DCFilter
{
public:
    int process()
    {
        DCBuffer buf;
        DCBuffer * doneBuf;
        int i;
        double before, after, elapsed;
        double MB;
        int packet_size = dcmpi_csnum(get_param("packet_size"));
        int num_packets = dcmpi_csnum(get_param("num_packets"));
        std::vector<DCBuffer*> allocated_bufs;
        for (i = 0; i < num_packets; i++) {
            allocated_bufs.push_back(new DCBuffer(packet_size));
            char setChar = (char)(i % 256);
            memset(allocated_bufs[i]->getPtr(), setChar, packet_size);
            allocated_bufs[i]->setUsedSize(packet_size);
        }
        before = dcmpi_doubletime();
        for (i = 0; i < num_packets; i++) {
            write_nocopy(allocated_bufs[i], "0");
        }
        doneBuf = read("1");
        after = dcmpi_doubletime();
        doneBuf->consume();
        elapsed = after - before;
        MB = (double)(packet_size * num_packets) / MB_1;
        char s[256];
        snprintf(s, sizeof(s),
                 "MB/sec %4.2f bytes %10s bufsize %d reps %d "
                 "seconds %2.2f\n",
                 MB/elapsed,
                 dcmpi_to_string(packet_size*num_packets).c_str(),
                 packet_size, num_packets, elapsed);
        cout << s;
        return 0;
    }
};

class bw2 : public DCFilter
{
    int some_member_var;
public:
    int process()
    {
        DCBuffer * buf;
        int i;
        int num_packets = dcmpi_csnum(get_param("num_packets"));
        for (i = 0; i < num_packets; i++) {
            buf = read("0");

#ifdef DEBUG // sanity check
            if (buf->getPtr()[0] != (char)(i%256)) {
                assert(0);
            }
#endif
            buf->consume();
        }
        DCBuffer doneBuf;
        doneBuf.pack("i", 0);
        write(&doneBuf, "1"); // (semantic-find-first-tag-by-name "doneBuf" (semantic-something-to-tag-table (current-buffer))) (semantic-analyze-current-context))

        // doneBuf. //(semantic-find-tags-by-name "doneBuf" (oref (semantic-analyze-current-context) localvariables))
        //
//         string s8;
//         std::string s7;
            // (setq s1 (semantic-analyze-current-context))
        return 0;
    }
};

dcmpi_provide2(bw1, bw2);
