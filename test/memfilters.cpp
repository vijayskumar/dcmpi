#include <dcmpi.h>

using namespace std;

class memWriter : public DCFilter
{
public:
    int process()
    {
        int i;
        int j;
        DCBuffer * work = get_init_filter_broadcast();
        std::string packet_size_and_num_packets;
        work->Extract(& packet_size_and_num_packets);
        std::vector<std::string> toks =
            dcmpi_string_tokenize(packet_size_and_num_packets);
        int packet_size = dcmpi_csnum(toks[0]);
        int num_packets = dcmpi_csnum(toks[1]);
        for (i = 0; i < num_packets; i++) {
            DCBuffer * b = new DCBuffer(packet_size);
            char * p = b->getPtr();
            for (j = 0; j < packet_size; j++) {
                p[j] = (char)(j%256);
            }
            b->setUsedSize(packet_size);
            writeany_nocopy(b);
        }
        return 0;
    }
};

class memReader : public DCFilter
{
    int process()
    {
        int i;
        int j;
        DCBuffer * work = get_init_filter_broadcast();
        std::string packet_size_and_num_packets;
        work->Extract(& packet_size_and_num_packets);
        std::vector<std::string> toks =
            dcmpi_string_tokenize(packet_size_and_num_packets);
        int num_packets = dcmpi_csnum(toks[1]);
        for (i = 0; i < num_packets; i++) {
            DCBuffer * b = readany();
            char * p = b->getPtr();
            for (j = 0; j < b->getUsedSize(); j++) {
                char expected = (char)(j%256);
                if (p[j] != expected) {
                    sleep(2);
                    std::cerr << "ERROR: expecting " << (int)expected
                              << ", got " << (int)p[j]
                              << ", j=" << j
                              << " at " << __FILE__ << ":" << __LINE__
                              << std::endl << std::flush;
                    std::cerr << *b;
                    // b->extended_print(cerr);
                    assert(0);
                }
            }
            b->consume();
            printf(".");
            fflush(stdout);
        }
        printf("\n");
        return 0;
    }
};

dcmpi_provide2(memWriter, memReader);
