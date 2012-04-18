#include <dcmpi.h>

using namespace std;

class policy_sender : public DCFilter
{
public:
    int process()
    {
        int4 i;
        for (i = 0; i < 20; i++) {
            DCBuffer buffer;
            buffer.pack("i", i);
            write(&buffer, "0");
        }
        cout << "policy_sender:  wrote 20 buffers\n";
        return 0;
    }
};

class policy_slow_guy : public DCFilter
{
public:
    int process()
    {
        DCBuffer * buf;
        int4 v;
        int4 num = 0;
        while (1) {
            dcmpi_doublesleep(0.2);
            buf = read_until_upstream_exit("0");
            if (!buf) {
                break;
            }
            buf->unpack("i", &v);
            cout << "policy_slow_guy: received buffer " << v << endl;
            buf->consume();
            num++;
        }

        cout << "policy_slow_guy: ending, read " << num << " packets" << endl;
        return 0;
    }
};

class policy_fast_guy : public DCFilter
{
public:
    int process()
    {
        DCBuffer * buf;
        int4 v;
        int4 num = 0;
        while (1) {
            buf = read_until_upstream_exit("0");
            if (!buf) {
                break;
            }
            buf->unpack("i", &v);
            cout << "policy_fast_guy: received buffer " << v << endl;
            buf->consume();
            num++;
        }

        cout << "policy_fast_guy: ending, read " << num << " packets" << endl;
        return 0;
    }
};

dcmpi_provide3(policy_sender, policy_slow_guy, policy_fast_guy);
