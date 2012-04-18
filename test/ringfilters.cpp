/***************************************************************************
    $Id: ringfilters.cpp,v 1.3 2005/03/01 19:14:00 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include <dcmpi.h>

using namespace std;

class ringfilter : public DCFilter
{
public:
    int process()
    {
        DCBuffer * in;
        DCBuffer out;
        in = read("0");
        std::string s;
        in->Extract(& s);
        in->consume();
        out.Append(s + "f" + get_param("myid") + " was here\n");
        write(&out, "0");
        return 0;
    }
};

dcmpi_provide1(ringfilter);
