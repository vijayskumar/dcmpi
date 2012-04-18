#include <dcmpi.h>

using namespace std;

class rcmd_filter : public DCFilter
{
    int process()
    {
        std::string rcmd = get_param("rcmd");
        return system(rcmd.c_str());
    }
};

dcmpi_provide1(rcmd_filter);
