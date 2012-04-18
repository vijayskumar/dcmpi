#include "dcmpi_util.h"

using namespace std;

char * appname = NULL;
void usage()
{
    printf("usage: [-systemwide] %s\n", appname);
    exit(EXIT_FAILURE);
}

std::string dcmpi_home_dir;

void config_runcommand()
{
    std::vector<std::string> in_MPI_versions;
    std::vector<int> out_MPI_versions_indexes;
    std::vector<std::string> out_MPI_versions;

    in_MPI_versions.push_back("pure sockets-based (default)");
#ifdef DCMPI_HAVE_MPI
    in_MPI_versions.push_back("mpich");
    in_MPI_versions.push_back("LAM/MPI");
    in_MPI_versions.push_back("MVAPICH (over rsh)");
    in_MPI_versions.push_back("MVAPICH (over ssh)");
    in_MPI_versions.push_back("mpich2/mpd (dcmpi manages daemons, max 1 dcmpi job at a time)");
    in_MPI_versions.push_back("mpich2/mpd (user manages daemons via mpdboot/mpdallexit)");
    in_MPI_versions.push_back("mpich2 + SMPD + ssh");
    in_MPI_versions.push_back("openmpi (doesn't seem to work ATM)");
    in_MPI_versions.push_back("other");
#endif
    
    dcmpi_bash_select_emulator(
        2, in_MPI_versions,
        "Please select your message-passing flavor to help determine the\n"
        "  proper run command for your communication layer.  This step writes\n"
        "  to your 'runcommand' file in your ~/.dcmpi directory.  If none of\n"
        "  the main options look right to you, then you can use the 'other'\n"
        "  choice to create your custom run command."
        "\n\n",
        "Abort",
        false, false,
        out_MPI_versions, out_MPI_versions_indexes);

    if (out_MPI_versions.empty()) {
        return;
    }
    std::string contents;

    if (out_MPI_versions_indexes[0] == 0) {
        contents =
            "# for pure sockets (non-MPI) execution\n"
            DCMPI_DEFAULT_RUN_COMMAND "\n";
    }
#ifdef DCMPI_HAVE_MPI
    else if (out_MPI_versions_indexes[0] == 1) {
        contents =
            "# for mpich\n"
            "mpirun -v -np %n -machinefile %h %D %e\n";
    }
    else if (out_MPI_versions_indexes[0] == 2) {
        contents =
            "# for lam\n"
            "mpiexec -machinefile %h -n %n %D %e\n";
    }
    else if (out_MPI_versions_indexes[0] == 3) {
        contents =
            "# for MVAPICH\n"
            "mpirun_rsh -rsh %D -np %n -hostfile %h %e\n";
    }
    else if (out_MPI_versions_indexes[0] == 4) {
        contents =
            "# for MVAPICH\n"
            "mpirun_rsh -ssh %D -np %n -hostfile %h %e\n";
    }
    else if (out_MPI_versions_indexes[0] == 5) {
        contents =
            "# for mpich2 (defaults)\n"
            "mpdboot -n %n -f %h; mpirun %D -machinefile %h -n %n %e; mpdallexit\n";
    }
    else if (out_MPI_versions_indexes[0] == 6) {
        contents =
            "# for mpich2 (user manages daemons)\n"
            "mpirun %D -machinefile %h -n %n %e\n";
    }
    else if (out_MPI_versions_indexes[0] == 7) {
        contents =
            "# for mpich2/SMPD/ssh\n"
            "dcmpi-mpich2-smpd-launcher -rsh %n %h %D %e\n";
    }
    else if (out_MPI_versions_indexes[0] == 8) {
        contents =
            "# for openmpi\n"
            "mpirun --no-daemonize -n %n -hostfile %h %D %e\n";
    }
    else if (out_MPI_versions_indexes[0] == 9) {
        printf("  Please enter your own launch string.  The following characters\n"
               "  are considered special:\n"
               "\n"
               "  %%n <-- number of nodes involved in the job\n"
               "  %%h <-- the host file containing hostnames of nodes in the job\n"
               "  %%D <-- an arbitrary string which will be filled in with the value\n"
               "         of the environment variable DCMPI_DEBUG at launch time\n\n");
        printf("  Enter your own launch string here: ");
        char line[512];
        fgets(line, sizeof(line), stdin);
        contents = line;
    }
#endif
    std::string filename = dcmpi_home_dir + "/runcommand";
    printf("  Writing to file %s...", filename.c_str());
    FILE * f;
    if ((f = fopen(filename.c_str(), "w")) == NULL) {
        std::cerr << "ERROR: opening file"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    if (fwrite(contents.c_str(), contents.size(), 1, f) < 1) {
        std::cerr << "ERROR: calling fwrite()"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    if (fclose(f) != 0) {
        std::cerr << "ERROR: calling fclose()"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    printf("done\n\n");
}

void config_graphviz()
{
    char line[256];
    std::string contents;
    printf("  Enable graphviz? (y or n): ");
    fgets(line, sizeof(line), stdin);
    if (line[0] == 'y') {
        contents +="enabled yes\n";
        printf("  Output filename (.ps or .dot file only): ");
        fgets(line, sizeof(line), stdin);
        contents += "output ";
        contents += line;
        contents += "# posthook scp %o foo:bar &\n";
    }
    else {
        contents =
            "enabled no\n"
            "# output .../dcmpi.ps\n"
            "# posthook scp %o foo:bar &\n";
    }
    std::string filename = dcmpi_home_dir + "/graphviz";
    printf("  Writing to file %s...", filename.c_str());
    FILE * f;
    if ((f = fopen(filename.c_str(), "w")) == NULL) {
        std::cerr << "ERROR: opening file"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    if (fwrite(contents.c_str(), contents.size(), 1, f) < 1) {
        std::cerr << "ERROR: calling fwrite()"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    if (fclose(f) != 0) {
        std::cerr << "ERROR: calling fclose()"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    printf("done\n\n");
}

void config_write_buffer_space()
{
    printf("  How many bytes to use per node for write buffering?\n");
    printf("  The amount given will be divided among all running filters.\n");
    printf("  You can give the number in bytes, or in shorthand notation, e.g.\n");
    printf("    500m <-- 500 megabytes\n");
    printf("    1g   <-- 1   gigabyte\n");
    printf("  Please enter the amount: ");
    char line[256];
    fgets(line, sizeof(line), stdin);
    std::string contents;
    std::string filename = dcmpi_home_dir+"/memory";
    std::map<std::string, std::string> pairs;
    if (dcmpi_file_exists(filename)) {
        pairs = file_to_pairs(filename);
    }
    std::string v = line;
    dcmpi_string_trim(v);
    pairs["write_buffer"] = v;
    printf("  Writing to file %s...", filename.c_str());
    pairs_to_file(pairs, filename);
    printf("done\n\n");
}

void config_proxy()
{
    printf("  Where is your proxy server located?  (This is only used for\n");
    printf("  connecting remote clusters together).\n");
    printf("\n");
    std::string proxy;
    while (1) {
        printf("  Please specify the location of your proxy server\n"
               "  here in HOSTNAME:PORT syntax: ");
        char buf[256];
        fgets(buf, sizeof(buf), stdin);
        if (strstr(buf, ":")) {
            proxy = buf;
            break;
        }
    }
    dcmpi_string_trim(proxy);
    proxy += "\n";
    std::string filename = dcmpi_home_dir + "/remote_proxy";
    FILE * f;
    if ((f = fopen(filename.c_str(), "w")) == NULL) {
        std::cerr << "ERROR: opening file"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    if (fwrite(proxy.c_str(), proxy.size(), 1, f) < 1) {
        std::cerr << "ERROR: calling fwrite()"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    if (fclose(f) != 0) {
        std::cerr << "ERROR: calling fclose()"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    printf("\n  Wrote file %s\n\n", filename.c_str());
}

void config_sockets_rsh(void)
{
    printf("  Please enter your desired remote shell command.\n");
    printf("  The default is " DCMPI_DEFAULT_SOCKET_RSH_COMMAND "\n");
    printf("\n");
    char buf[256];
    std::string response;
    while (1) {
        printf("  Enter the command here: ");
        fgets(buf, sizeof(buf), stdin);
        response = buf;
        dcmpi_string_trim(response);
        if (response.size() > 0) {
            break;
        }
    }

    FILE * f;
    std::string filename = dcmpi_home_dir + "/socket";
    std::map<std::string, std::string> pairs;
    if ((f = fopen(filename.c_str(), "r")) == NULL) {
        ;
    }
    else {
        if (fclose(f) != 0) {
            std::cerr << "ERROR: errno=" << errno << " calling fclose()"
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
        pairs = file_to_pairs(filename);
    }
    pairs["rsh"] = response;
    pairs_to_file(pairs, filename);
    printf("\n  Wrote file %s\n\n", filename.c_str());
}

int main(int argc, char * argv[])
{
    appname = argv[0];
    dcmpi_home_dir = dcmpi_get_home();

    while (argc > 1) {
        if (!strcmp(argv[1], "-h")) {
            usage();
        }
        else if (!strcmp(argv[1], "-systemwide")) {
            dcmpi_home_dir = get_systemwide_config_root();
            if (dcmpi_home_dir == "") {
                std::cerr << "ERROR: systemwide dcmpi install not found\n";
                exit(1);
            }
            if (!dcmpi_file_exists(dcmpi_home_dir)) {
                std::cout << "NOTE:  creating " << dcmpi_home_dir
                          << endl;
                dcmpi_mkdir_recursive(dcmpi_home_dir);
            }
        }
        else {
            break;
        }
        dcmpi_args_shift(argc, argv);
    }

    if ((argc-1) != 0) {
        usage();
    }

    while (1) {
        std::vector<std::string> top_level_options;
        std::vector<std::string> top_level_options_out;
        std::vector<int> top_level_options_out_indexes;

        top_level_options.push_back("configure message passing (MPI or socket) run command");
        top_level_options.push_back("configure write buffer space");
        top_level_options.push_back("configure graphviz support");
        top_level_options.push_back("configure proxy server (for remote cluster jobs)");
        top_level_options.push_back("configure socket-based remote shell command");
    
        dcmpi_bash_select_emulator(
            0, top_level_options,
            "dcmpiconfig (top level)\n"
            "\nPlease select from the following options:\n\n",
            "quit",
            false, false,
            top_level_options_out, top_level_options_out_indexes);

        if (top_level_options_out_indexes.empty()) {
            break;
        }
        else if (top_level_options_out_indexes[0] == 0) {
            config_runcommand();
        }
        else if (top_level_options_out_indexes[0] == 1) {
            config_write_buffer_space();
        }
        else if (top_level_options_out_indexes[0] == 2) {
            config_graphviz();
        }
        else if (top_level_options_out_indexes[0] == 3) {
            config_proxy();
        }
        else if (top_level_options_out_indexes[0] == 4) {
            config_sockets_rsh();
        }
    }
}
