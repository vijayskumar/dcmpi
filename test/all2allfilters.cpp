#include <dcmpi.h>

using namespace std;

class all2all : public DCFilter
{
public:
    int process()
    {
        int i, j;
        int packet_size = dcmpi_csnum(get_param("packet_size"));
        int num_packets = dcmpi_csnum(get_param("num_packets"));
        int filter_count = atoi(get_param("filter_count").c_str());
        int myid = atoi(get_param("myid").c_str());
        std::string localhost = dcmpi_get_hostname();

        // wait for sync with all other filters
        DCBuffer readyBuf;
        write(&readyBuf, "ready");
        DCBuffer * goBuf = read("ready");
        goBuf->consume();

        for (i = 0; i < num_packets; i++) {
            int dest = myid;
            for (j = 0; j < filter_count; j++) {
                if (dest != myid) {
                    DCBuffer * out = new DCBuffer(packet_size);
                    out->setUsedSize(packet_size);
                    write_nocopy(out, dcmpi_to_string(dest));
                }
                dest++;
                if (dest == filter_count) {
                    dest = 0;
                }
            }
            for (j = 1; j < filter_count; j++) {
                DCBuffer * in = readany();
                assert(in);
                in->consume();
            }
        }
        return 0;
    }
};

dcmpi_provide1(all2all);
