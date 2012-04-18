#include <dcmpi.h>

using namespace std;

class labelSender : public DCFilter
{
    int process()
    {
        int4 i;
        int4 reps;
        for (reps = 0; reps < 3; reps++) {
            DCBuffer header;
            header.pack("i",0);
            std::string label = "foo";
            label += dcmpi_to_string(reps);
            write(&header, "0", label);
            for (i = 1; i <= 3; i++) {
                DCBuffer body;
                body.pack("i", i);
                write(&body, "0", label);
            }
        }
        return 0;
    }
};

class labelReceiver : public DCFilter
{
    int process()
    {
        int4 i;
        int packets_in = 0;
        DCBuffer * in;
        std::string port;
        while ((in = readany(&port))) {
            delete in;
            packets_in++;
            for (i = 0; i < 3; i++) {
                in = read_until_upstream_exit(port);
                assert(in);
                packets_in++;
                delete in;
            }
        }
        cout << "labelReceiver:  received "
             << packets_in << " packets\n" << flush;
        assert((packets_in % 4)==0);
        return 0;
    }
};

dcmpi_provide2(labelSender, labelReceiver);
