#include <stdio.h>

#include <dcmpi.h>

using namespace std;

class dcmpi_recon_filter : public DCFilter
{
public:
    int get_CPU_count()
    {
        return sysconf(_SC_NPROCESSORS_ONLN);
// #ifdef __linux__
//         int CPU_count = 0;
//         FILE * CPU_file = fopen("/proc/cpuinfo","r");
//         assert(CPU_file);
//         char buffer[256];
//         while (fgets(buffer, sizeof(buffer), CPU_file)) {
//             if (strncmp(buffer, "processor", 9) == 0) {
//                 CPU_count++;
//                 cout << "saw another CPU on " << dcmpi_get_hostname()
//                      << endl;
//             }
//         }
//         return CPU_count;
// #else
// #error unknown architecture
// #endif
    }
    int8 get_physical_memory()
    {
#ifdef DCMPI_CANNOT_DISCOVER_PHYSICAL_MEMORY
        std::cerr << "WARNING: cannot determing physical memory for this machine ("
                  << dcmpi_get_hostname() << ")" << endl
                  << "Defaulting to infinite memory during recon." << endl;
        return ~((int8)0);
#else
        int8 pages = (int8)sysconf(_SC_PHYS_PAGES);
        int8 page_size = (int8)sysconf(_SC_PAGE_SIZE);
        cout << "pages " << pages << " page_size " << page_size
             << endl;
        return pages*page_size;
#endif
    }
    int process()
    {
        int4 CPU_count = get_CPU_count();
        int8 memory_count = get_physical_memory();
        // return back to console
        DCBuffer out_value;
        out_value.pack("il", CPU_count, memory_count);
        write(&out_value, "0");
        return 0;
    }
};

class dcmpi_availmem : public DCFilter
{
public:
    int process()
    {
        int8 page_size = (int8)sysconf(_SC_PAGE_SIZE);
//         int8 total_pages = (int8)sysconf(_SC_PHYS_PAGES);
//         int8 total_available_memory = page_size * total_pages;

//         {
//             int8 avail = total_available_memory;
//             uint u;
//             // allocate the amount of physical memory all at once, and touch
//             // every page, which will hopefully report a more accurate amount
//             // of available physical memory
//             std::vector<char*> bufs;
//             std::vector<size_t> sizes;
//             size_t allocsz;
//             cout << "allocating " << total_available_memory << " bytes" << endl;
//             while (avail) {
//                 if (avail >= GB_1) {
//                     allocsz = GB_1;
//                 }
//                 else {
//                     allocsz = avail;
//                 }
//                 avail -= allocsz;
//                 cout << "sub-allocating " << allocsz << " bytes" << endl;
//                 bufs.push_back(new char[allocsz]);
//                 sizes.push_back(allocsz);
//             }
//             for (u = 0; u < bufs.size(); u++) {
//                 size_t goal = sizes[u];
//                 size_t i;
//                 for (i=0; i < goal; i += (size_t)page_size) {
//                     bufs[u][i]++;
//                 }
//             }
//             dcmpi_doublesleep(1.3);
//             cout << "freeing " << total_available_memory << " bytes" << endl;
//             for (u = 0; u < bufs.size(); u++) {
//                 delete[] bufs[u];
//             }
//         }

        int8 available_pages = (int8)sysconf(_SC_AVPHYS_PAGES);
        DCBuffer out_value;
        out_value.pack("l", (int8)(available_pages*page_size));
        write(&out_value, "0");
        return 0;
    }
};

dcmpi_provide2(dcmpi_recon_filter,dcmpi_availmem);
