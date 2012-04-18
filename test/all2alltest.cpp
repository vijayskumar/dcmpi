#include <dcmpi.h>

using namespace std;

char * appname = NULL;
void usage()
{
    printf("usage: %s <hostfile> <packet_size> <num_packets>\n", appname);
    exit(EXIT_FAILURE);
}


int main(int argc, char * argv[])
{
    appname = argv[0];
    if (((argc-1) != 3)) {
        usage();
    }

    DCLayout layout;
    layout.use_filter_library("liball2allfilters.so");
    DCFilterInstance console("<console>", "console");
    layout.add(console);
    int lines = dcmpi_file_lines(argv[1]);
    int i, j;
    std::vector<DCFilterInstance*> filters;
    for (i = 0; i < lines; i++) {
        DCFilterInstance * f = new DCFilterInstance(
            "all2all", "all2all_" +  dcmpi_to_string(i));
        filters.push_back(f);
        layout.add(f);
        f->set_param("myid", dcmpi_to_string(i));
    }
    for (i = 0; i < lines; i++) {
        for (j = 0; j < lines; j++) {
            if (i != j) {
                layout.add_port(filters[i], dcmpi_to_string(j),
                                filters[j], "incoming");
            }
        }
        layout.add_port(filters[i], "ready", &console, "ready");
        layout.add_port(&console, "ready", filters[i], "ready");
    }
    layout.set_param_all("packet_size", argv[2]);
    layout.set_param_all("num_packets", argv[3]);
    layout.set_param_all("filter_count", dcmpi_to_string(lines));
    layout.set_exec_host_pool_file(argv[1]);

    DCFilter * c = layout.execute_start();

    // wait till all are ready
    cout << "waiting for all filters to be ready..." << flush;
    for (i = 0; i < lines; i++) {
        DCBuffer * b = c->read("ready");
        b->consume();
    }
    DCBuffer goBuf;
    for (i = 0; i < lines; i++) {
        c->write(&goBuf, "ready");
    }
    cout << "done\nall filters are ready, test is commencing\n";
    double tstart = dcmpi_doubletime();
    int rc = layout.execute_finish();
    double tstop = dcmpi_doubletime();
    double elapsed = tstop - tstart;
    double MB = ((dcmpi_csnum(argv[2]) * (dcmpi_csnum(argv[3])) *
                  (lines-1) * lines)
                 / (1024.0*1024.0));
    double MB_per_sec =  MB / elapsed;
    cout << "MB/sec " << MB_per_sec << " MB " << MB
         << " elapsed time " << elapsed
         << endl;
    return rc;
}
