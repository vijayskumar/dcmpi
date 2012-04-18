/***************************************************************************
    $Id: compfilters.cpp,v 1.2 2006/05/09 09:16:28 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include "dcmpi.h"

using namespace std;

class num_feeder : public DCFilter
{
public:
    int process()
    {
        int4 i;
        for (i = 0; i < 100; i++) {
            DCBuffer * outb = new DCBuffer;
            outb->Append(i);
            writeany_nocopy(outb);
        }

        return 0;
    }
};

class averager : public DCFilter
{
public:
    DCLayout * get_composite_layout(std::vector<std::string> cluster_hosts)
    {
        DCLayout * layout = new DCLayout;
        DCFilterInstance * adder = new DCFilterInstance("adder","adder1");
        DCFilterInstance * divider = new DCFilterInstance("divider","divider1");
        adder->bind_to_host(cluster_hosts[0]);
        divider->bind_to_host(cluster_hosts[cluster_hosts.size()-1]);
        layout->add(adder);
        layout->add(divider);
        layout->add_port(adder, "0", divider, "0");
        adder->designate_as_inbound_for_composite();
        divider->designate_as_outbound_for_composite();
        return layout;
    }
};

class adder : public DCFilter
{
    int process()
    {
        DCBuffer * outb = new DCBuffer;
        DCBuffer * inb;
        int4 sum = 0;
        int4 val = 0;
        int4 vals = 0;
        while ((inb = readany())) {
            inb->unpack("i", &val);
            sum += val;
            vals++;
            inb->consume();
        }
        outb->Append(sum);
        outb->Append(vals);
        writeany_nocopy(outb);
        return 0;
    }
};

class divider : public DCFilter
{
    int process()
    {
        int4 sum = 0;
        int4 vals = 0;
        int4 sum_total = 0;
        int4 vals_total = 0;
        DCBuffer * inb;
        while ((inb = readany())) {
            inb->Extract(&sum);
            inb->Extract(&vals);
            inb->consume();
            sum_total += sum;
            vals_total += vals;
            cout << "divider: sum=" << sum;
            cout << ", vals=" << vals << endl;
        }
        DCBuffer out;
        out.pack("i", sum_total / vals_total);
        writeany(&out);
        return 0;
    }
};

class resultreader : public DCFilter
{
public:
    int process()
    {
        DCBuffer * inb;
        while ((inb = readany())) {
            int4 result = 0;
            inb->Extract(&result);
            cout << "resultreader: result from average is " << result << endl;
            inb->consume();
        }
        return 0;
    }
};

dcmpi_provide5(num_feeder, averager, adder, divider, resultreader);
