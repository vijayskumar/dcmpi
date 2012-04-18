#include <dcmpi.h>

using namespace std;

class selfwrite : public DCFilter
{
public:
    int process()
    {
        DCBuffer buf;
        buf.pack("iii", 1, 2, 3);
        write(&buf, "0");
        DCBuffer * inbuf = read("0");
        int i1, i2, i3;
        inbuf->unpack("iii", &i1, &i2, &i3);
        cout << i1 << " " << i2 << " " << i3 << endl;
        return 0;
    }
};

dcmpi_provide1(selfwrite);
