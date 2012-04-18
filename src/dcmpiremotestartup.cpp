#include "dcmpi_internal.h"

#include "dcmpi_util.h"

#include <dcmpi.h>

using namespace std;

char * appname = NULL;
void usage()
{
    printf("usage: %s <args...>\n", appname);
    printf("(for internal use only)\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char * argv[])
{
    if ((argc-1) < 2) {
        appname = argv[0];
        usage();
    }
    std::map<std::string, std::string> launch_params;
    std::vector<std::string> temp_files;
    std::vector<std::string> hosts;

    assert((argc-1) % 2 == 0);

    int i;
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h")) {
            hosts.push_back(argv[i+1]);
            i += 1;
        }
        else {
            launch_params[argv[i]] = argv[i+1];
            i += 1;
        }
    }

    std::string run_command = get_raw_run_command();
    std::string runtime = get_dcmpiruntime_runtime(run_command);
    std::string suffix = get_dcmpiruntime_suffix(run_command);
    run_command_finalize(runtime,
                         suffix,
                         hosts,
                         launch_params,
                         run_command,
                         temp_files);
//    cout << "dcmpiremotestartup: executing '" << run_command << "'" << endl;
//     run_command = "gdbe " + run_command;
    int rc = system(run_command.c_str());
    for (i = 0; i < (int)temp_files.size(); i++) {
        remove(temp_files[i].c_str());
    }
    if (rc) {
        std::cerr << "ERROR: '" << run_command
                  << "' returned " << rc
                  << " on host " << dcmpi_get_hostname()
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
    }
//     else {
//         cout << "dcmpiremotestartup: returned 0\n";
//     }
    return rc;
}
