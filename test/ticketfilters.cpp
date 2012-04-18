#include <dcmpi.h>

using namespace std;

class ticketSender : public DCFilter
{
    int process()
    {
        int4 i;
        int4 reps;
        for (reps = 0; reps < 3; reps++) {
            DCBuffer header;
            header.pack("i",0);
            write(&header, "0");
            int ticket = get_last_write_ticket("0");
            for (i = 0; i < 3; i++) {
                DCBuffer body;
                body.pack("i", i);
                write(&body, "0", ticket);
            }
        }
        return 0;
    }
};

class ticketReceiver : public DCFilter
{
    int process()
    {
        int4 i;
        int packets_in = 0;
        DCBuffer * in;
        std::string port;
        int copy_rank = this->get_copy_rank();
        while ((in = readany(&port))) {
            packets_in++;
            for (i = 0; i < 3; i++) {
                in = read_until_upstream_exit(port);
                assert(in);
                packets_in++;
            }
        }
        dcmpi_doublesleep(copy_rank * 0.1);
        cout << "ticketReceiver: copy_rank " << copy_rank
             << ": received " << packets_in << " packets\n" << flush;
        assert((packets_in % 4)==0);
        return 0;
    }
};

dcmpi_provide2(ticketSender, ticketReceiver);
