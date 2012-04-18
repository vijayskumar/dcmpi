#ifndef MEMUSAGE_H
#define MEMUSAGE_H

#include "dcmpi_clib.h"
#include "dcmpi_util.h"
#include "dcmpi_home.h"
#include "DCCond.h"

/* mem accumulation spots:

-filter starts a write()
-MPI_Test completes an incoming req
-console recvs buf from MPIToConsoleThread
-MPIFromConsoleThread recvs buf from console

   mem reduction spots:
-filter completes a read()
-MPI_Test completes an outgoing req
-console delivers buf to MPIFromConsoleThread
-MPIToConsoleThread delivers buf to console

*/
class MemUsage
{
public:
    MemUsage(int8 max_bytes, std::string descriptive_context);
    void accumulate(int bytes);
    void reduce(int bytes);
    bool accumulate_if_not_exceeding_limit(int bytes);
    int8 get_current_usage() {
        int8 val;
        cond.acquire();
        val = current_usage;
        cond.release();
        return val;
    }
private:
    DCCond cond;
    int8 memory_threshold;
    int8 current_usage;
    std::string localhostname;
    std::string _context;
    MemUsage(){}
};

#ifdef DCMPI_CANNOT_DISCOVER_PHYSICAL_MEMORY
inline int8 get_phys_memory()
{
//     std::cerr << "WARNING: cannot determine physical memory for this machine.\n"
//               << "  In order to properly control how much memory dcmpi uses\n"
//               << "  for buffering of DCBuffer objects, in order to prevent\n"
//               << "  memory from growing out of control, add a line like\n"
//               << "\n"
//               << "write_buffer 512m\n"
//               << "\n"
//               << "  in the file "
//               << dcmpi_get_home() + "/memory"
//               << "\n"
//               << "Defaulting to infinite memory for write buffering.\n"
//               << std::endl << std::flush;
//     return ~((int8)0);
    return MB_128;
}
#else
inline int8 get_phys_memory()
{
    return
        (int8)sysconf(_SC_PHYS_PAGES) *
        (int8)sysconf(_SC_PAGE_SIZE);
}
#endif

inline int8 get_user_specified_write_buffer_space()
{
    int8 user_specified_memory;
    std::vector<std::string> files = get_config_filenames("memory");
    std::string memfile;
    if (files[0] != "") {
        memfile = files[0];
    }
    if (files[1] != "") {
        memfile = files[1];
    }
    if (memfile != "" && fileExists(memfile)) {
        std::map<std::string, std::string> pairs = file_to_pairs(memfile);
        if (pairs.count("write_buffer") > 0) {
            user_specified_memory = (int8)dcmpi_csnum(
                pairs["write_buffer"]);
        }
        else {
            user_specified_memory = (get_phys_memory() / 4) * 3;
        }
    }
    else {
        user_specified_memory = (get_phys_memory() / 4) * 3;
    }
    return user_specified_memory;
}

#endif /* #ifndef MEMUSAGE_H */
