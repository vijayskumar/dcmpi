#include "dcmpi.h"

using namespace std;

class producer : public DCFilter
{
public:
    int process()
    {
        int compress = get_param_as_int("compress");
        int i;
        for (i = 0; i < 10000; i++) {
            DCBuffer * outb = new DCBuffer(i);
            memset(outb->getPtr(), '*', i);
            outb->setUsedSize(i);
            if (compress) {
                outb->compress();
            }
            std::cout << "wrote bufsize: " << outb->getUsedSize() << endl;
            write_nocopy(outb, "0");
        }
        return 0;
    }
};

class consumer : public DCFilter
{
public:
    int process() {
        double t1 = dcmpi_doubletime();
        int compress = get_param_as_int("compress");
        int pkts = 0;
        while (1) {
            DCBuffer * in = read_until_upstream_exit("0");
            if (!in) {
                break;
            }
            std::cout << "read bufsize:  " << in->getUsedSize() << endl;
            if (compress) {
                in->decompress();
            }
            in->consume();
            pkts++;
        }
        std::cout << "got " << pkts << " packets\n";
        double elapsed = dcmpi_doubletime() - t1;
        std::cout << "elapsed: " << elapsed << endl;
        return 0;
    }
};

dcmpi_provide2(producer, consumer);

