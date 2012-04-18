#include <dcmpi.h>

#include <iomanip>

using namespace std;

class bidibwsender : public DCFilter
{
public:
    int process()
    {
        DCBuffer buf;
        int i;
        double before, after, elapsed;
        double MB;
        int packet_size = dcmpi_csnum(get_param("packet_size").c_str());
        int num_packets = dcmpi_csnum(get_param("num_packets").c_str());
        std::vector<DCBuffer*> allocated_bufs;
        bool collectsFinalResults =
            (get_param("collectsFinalResults") == "true");
        for (i = 0; i < num_packets; i++) {
            allocated_bufs.push_back(new DCBuffer(packet_size));
            memset(allocated_bufs[i]->getPtr(), 0, packet_size);
            allocated_bufs[i]->setUsedSize(packet_size);
        }
        if (collectsFinalResults) {
            delete read("ack");
            delete read("ack");
            before = dcmpi_doubletime();
        }
        for (i = 0; i < num_packets; i++) {
            write_nocopy(allocated_bufs[i], "data");
        }
        if (collectsFinalResults) {
            // wait for 2 acks
            delete read("ack");
            delete read("ack");
            after = dcmpi_doubletime();
            elapsed = after - before;
            MB = (double)(packet_size * num_packets * 2) / MB_1;
            cout.precision(2);
            char s[256];
            snprintf(s, sizeof(s),
                     "MB/sec %4.2f bytes %10s bufsize %d reps %d "
                     "seconds %2.2f\n",
                     MB/elapsed,
                     dcmpi_to_string(packet_size*num_packets*2).c_str(),
                     packet_size, num_packets*2, elapsed);
            cout << s;
        }
        return 0;
    }
};

class bidibwreceiver : public DCFilter
{
public:
    int process()
    {
        DCBuffer ackBuf;
        DCBuffer * buf;
        int i;
        write(&ackBuf, "ack");
        int num_packets = atoi(get_param("num_packets").c_str());
        for (i = 0; i < num_packets; i++) {
            buf = read("data");
            buf->consume();
        }
        write(&ackBuf, "ack");
        return 0;
    }
};

dcmpi_provide2(bidibwsender, bidibwreceiver);
