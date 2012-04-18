/***************************************************************************
    $Id: consolefilters.cpp,v 1.5 2006/06/20 18:44:22 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include <dcmpi.h>

using namespace std;

class ca : public DCFilter
{
public:
    int process()
    {
        DCBuffer * buf;
        for (int i = 0; i < 10; i++) {
            buf = read("0");
            std::string s;
            buf->Extract(&s);
            buf->consume();
            DCBuffer * out = new DCBuffer;
            cout << "ca: received " << s << " on host " << dcmpi_get_hostname()
                 << endl;
            out->Append("ca: passing on '" + s + "'");
            writeany_nocopy(out);
        }
        cout << "ca: ending" << endl;
        return 0;
    }
};

class cb : public DCFilter
{
public:
    int process()
    {
        DCBuffer * buf;
        for (int i = 0; i < 10; i++) {
            buf = read("0");
            std::string s;
            buf->Extract(&s);
            buf->consume();
            DCBuffer * out = new DCBuffer;
            cout << "cb: received " << s << " on host " << dcmpi_get_hostname()
                 << endl;
            out->Append("cb: passing on '" + s + "'");
            dcmpi_doublesleep(0.1);
            writeany_nocopy(out);
        }
        cout << "cb: ending" << endl;
        return 0;
    }
};

class console_receiver : public DCFilter
{
public:
    int process()
    {
        DCBuffer * in;
        while ((in = readany())!=NULL) {
            delete in;
        }
        std::cout << "finished on " << dcmpi_get_hostname() << endl;
        return 0;
    }
};

dcmpi_provide3(ca, cb, console_receiver);
