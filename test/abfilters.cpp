/***************************************************************************
    $Id: abfilters.cpp,v 1.6 2005/07/07 17:30:44 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include <dcmpi.h>

using namespace std;

class a : public DCFilter
{
public:
    int process()
    {
        int i;
        double d;
        int4 num = 10;
        std::string s;
        if (has_param("num_packets")) {
            num = atoi(get_param("num_packets").c_str());
        }
        cout << "a: starting\n";
        for (i = 0; i < num; i++) {
            DCBuffer buf(MB_1); // make sure it doesn't write 1MB
            d = (double)i + 0.1;
            buf.pack("d", d);
            write(&buf, "0");
        }
        cout << "a: ending on " << dcmpi_get_hostname() << endl;

        return 0;
    }
};

class b : public DCFilter
{
public:
    int process()
    {
        DCBuffer * buf;
        double d;
        cout << "b: starting" << endl;
        while ((buf = readany()) != NULL) {
            buf->unpack("d", &d);
            cout << "b: received " << d << endl;
            if (buf->getExtractAvailSize()) {
                cerr << "ERROR:  b: buf has " << buf->getExtractAvailSize()
                     << " bytes left after unpacking element " << d << endl;
                return -1;
            }
            buf->consume();
        }

        cout << "b: ending on " << dcmpi_get_hostname() << endl;
        return 0;
    }
};

dcmpi_provide2(a,b);
