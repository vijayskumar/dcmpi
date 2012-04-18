/***************************************************************************
    $Id: overwritefilters.cpp,v 1.2 2005/03/01 19:14:00 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include <dcmpi.h>

using namespace std;

class overwrite1 : public DCFilter
{
public:
    int process()
    {
        int4 i;
        int4 num = 10;
        cout << "overwrite1: starting, writing " << num << " packets\n";
        for (i = 0; i < num; i++) {
            DCBuffer buf;
            buf.pack("i", i);
            write(&buf, "0");
        }
        cout << "overwrite1: ending" << endl;

        return 0;
    }
};

class overwrite2 : public DCFilter
{
public:
    int process()
    {
        DCBuffer * buf;
        int4 i;
        int4 v;
        int4 num = 9;
        sleep(2);
        for (i = 0; i < num; i++) {
            buf = read("0");
            buf->unpack("i", &v);
            cout << "overwrite2: received " << v << endl;
            buf->consume();
        }

        cout << "overwrite2: ending, read " << num << " packets" << endl;
        return 0;
    }
};

dcmpi_provide2(overwrite1,overwrite2);
