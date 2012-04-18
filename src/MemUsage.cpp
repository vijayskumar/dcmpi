#ifdef DCMPI_HAVE_PYTHON
#include <Python.h>
#endif

#include "dcmpi.h"

#include "MemUsage.h"

#include "dcmpi_socket.h"

using namespace std;

MemUsage::MemUsage(int8 max_bytes, std::string context)
{
    memory_threshold = max_bytes;
    if (dcmpi_verbose()) {
        std::cerr << "memory_threshold set to "
                  << memory_threshold << endl;
    }
    current_usage = 0;
    localhostname = dcmpi_get_hostname();
    _context = context;
}

void MemUsage::accumulate(int bytes)
{
    if (bytes > memory_threshold) {
        std::cerr << "ERROR: an attempt was made by ("
                  << _context << ") to write "
                  << bytes << " bytes, yet the user memory threshold "
                  << "per filter is " << memory_threshold
                  << " bytes; the consequences are that this "
                  << "write will wait forever, since there "
                  << "will never be enough memory available.  Please fix "
                  << "this by either writing more smaller buffers instead "
                  << "of larger ones, or by increasing the machine user memory "
                  << "threshold by increasing the value in the file "
                  << dcmpi_get_home() << "/memory"
                  << std::endl << std::flush;
        exit(1);
    }
    int8 b = (int8)bytes;
    cond.acquire();
    while ((current_usage+b) > memory_threshold) {
        cond.wait();
    }
    current_usage += b;
    if (dcmpi_verbose()) {
        for (int i = 0; i < dcmpi_num_total_filters+1; i++) {
            if (this==MUID_MemUsage[i]) {
                std::string name;
                if (i == dcmpi_num_total_filters) {
                    name = "dcmpiruntime on ";
                }
                else {
                    name = "gftid " + dcmpi_to_string(i) + " on ";
                }
                char buf[256];
                sprintf(buf, "%f", dcmpi_doubletime());
                std::cerr << name << localhostname
                          << ": accumulate(), usage now "
                          << setw(8) << current_usage << setw(0) << " at "
                          << buf << std::endl;
                break;
            }
        }
    }
    cond.release();
}

void MemUsage::reduce(int bytes)
{
    cond.acquire();
    current_usage -= bytes;
    assert(current_usage >= 0);
    if (dcmpi_verbose()) {
        for (int i = 0; i < dcmpi_num_total_filters+1; i++) {
            if (this==MUID_MemUsage[i]) {
                std::string name;
                if (i == dcmpi_num_total_filters) {
                    name = "dcmpiruntime on ";
                }
                else {
                    name = "gftid " + dcmpi_to_string(i) + " on ";
                }
                char buf[256];
                sprintf(buf, "%f", dcmpi_doubletime());
                std::cerr << name << localhostname
                          << ": reduce(),     usage now "
                          << setw(8) << current_usage << setw(0) << " at "
                          << buf << std::endl;
                break;
            }
        }
    }
    cond.notifyAll();
    cond.release();
}

bool MemUsage::accumulate_if_not_exceeding_limit(int bytes)
{
    bool out;
    int8 b = (int8)bytes;
    cond.acquire();
    if ((current_usage+b) > memory_threshold) {
        out = false;
    }
    else {
        out = true;
        current_usage += b;
        if (dcmpi_verbose()) {
            for (int i = 0; i < dcmpi_num_total_filters+1; i++) {
                if (this==MUID_MemUsage[i]) {
                    std::string name;
                    if (i == dcmpi_num_total_filters) {
                        name = "dcmpiruntime on ";
                    }
                    else {
                        name = "gftid " + dcmpi_to_string(i) + " on ";
                    }
                    std::cerr << name << localhostname
                              << ": accumulate_if...(), usage now "
                              << current_usage << std::endl;
                    break;
                }
            }
        }
    }
    cond.release();
    return out;
}
