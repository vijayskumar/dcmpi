/***************************************************************************
    $Id: DCLayout.cpp,v 1.85 2006/09/29 16:45:24 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include "dcmpi_clib.h"

#include "dcmpi.h"

#ifdef DCMPI_HAVE_PCRE
#include <pcre.h>
#endif

#include "DCFilterRegistry.h"
#include "ConsoleFromMPIThread.h"
#include "ConsoleToMPIThread.h"
#include "DCFilterExecutor.h"
#include "DCMPIPacket.h"
#include "MultiOutportManager.h"
#include "MultiPort.h"
#include "Queue.h"
#include "QueueWriter.h"
#include "ResolvedMultiPort.h"
#include "Ring.h"
#include "dcmpi_socket.h"
#include "dcmpi_globals.h"
#include "DCFileLock.h"

using namespace std;

void DCLayout::common_init()
{
    filter_reg = new DCFilterRegistry;
    init_filter_broadcast = NULL;

    console_to_mpi_queue = NULL;
    console_to_mpi_thread = NULL;
    console_from_mpi_thread = NULL;
    console_filter = NULL;
    execute_start_called = false;

    localhost = dcmpi_get_hostname();
    localhost_shortname = dcmpi_get_hostname(true);

//     add_propagated_environment_variable("DCMPI_HOME");
    add_propagated_environment_variable("DCMPI_VERBOSE");
    add_propagated_environment_variable("DCMPI_MAINLOOP_SLEEPTIMES");
    add_propagated_environment_variable("DCMPI_REMOTE_OUTPUT");
    add_propagated_environment_variable("DCMPI_STATS");
    add_propagated_environment_variable("DCMPI_GDBLAUNCHER");
    add_propagated_environment_variable("DCMPI_JIT_GDB");
    add_propagated_environment_variable("DCMPI_FORCE_DISPLAY");
#ifdef DCMPI_HAVE_JAVA

    // this may cause a problem:  /nfs/1/home/rutt/dev/ocvm/../foo.jar doesn't
    // necessarily work elsewhere...so be careful how you set this variable.
    // one alternative is to use the ~/.dcmpi/dcmpi_java_classpath config file
    // instead.
    add_propagated_environment_variable("DCMPI_JAVA_CLASSPATH");
    
    add_propagated_environment_variable("DCMPI_JAVA_EXTRA_STARTUP_ARGS");
    add_propagated_environment_variable("DCMPI_JAVA_REMOTE_DEBUG_PORT");
    add_propagated_environment_variable("DCMPI_JAVA_MINIMUM_HEAP_SIZE");
    add_propagated_environment_variable("DCMPI_JAVA_MAXIMUM_HEAP_SIZE");
#endif
}

DCLayout::~DCLayout()
{
    if (execute_start_called) {
        std::cerr << "ERROR: DCLayout::execute_start(), but "
                  << "not DCLayout::execute_finish(), was called."
                  << std::endl << std::flush;
        exit(1);
    }
    clear_deserialized_filter_instances();
    delete filter_reg;
    delete init_filter_broadcast;
}

void DCLayout::use_filter_library(const std::string & library_name)
{
    filter_reg->add_filter_lib(library_name);
}

void DCLayout::clear_expansion_filter_instances()
{
    uint u;
    for (u = 0; u < expansion_filter_instances.size(); u++) {
        delete expansion_filter_instances[u];
    }
    expansion_filter_instances.clear();
}

void DCLayout::clear_deserialized_filter_instances()
{
    for (uint u = 0; u < deserialized_filter_instances.size(); u++) {
        delete deserialized_filter_instances[u];
    }
    deserialized_filter_instances.clear();
}

std::string DCLayout::to_graphviz()
{
    std::string s = "digraph dcmpi_layout {\n";
    s += "  label=\"dcmpi layout at " + dcmpi_get_time() + "\";\n";
    std::map<std::string, std::vector<DCFilterInstance*> > host_to_filters;
    uint u;

    // determine if all of the filters have a single host that they are bound
    // to.  if so, then output a host-specific graphviz layout.
    bool do_host_specific_graphviz = true;
    for (u = 0; u < filter_instances.size(); u++) {
        DCFilterInstance * fi = filter_instances[u];
        if (fi->dcmpi_host.size() > 0) {
            if (host_to_filters.count(fi->dcmpi_host) == 0) {
                std::vector<DCFilterInstance*> hostvec;
                host_to_filters[fi->dcmpi_host] = hostvec;
            }
            host_to_filters[fi->dcmpi_host].push_back(fi);
        }
        else {
            do_host_specific_graphviz = false;
            break;
        }
    }

    std::string instance_text;
    std::vector<std::string> all_connections;
    if (do_host_specific_graphviz) {
        int clusterID = 0;
        std::map<std::string, std::vector<DCFilterInstance*> >::iterator it;
        for (it = host_to_filters.begin(); it != host_to_filters.end(); it++) {
            std::string host = it->first;
            const std::vector<DCFilterInstance*> & instances = it->second;
            s += "  subgraph cluster_" + dcmpi_to_string(clusterID++) + " {\n";
            s += "    style=filled;\n";
            s += "    color=lightgrey;\n";
            s += "    fontcolor=blue;\n";
            s += "    node [style=filled,color=white];\n";
            for (u = 0; u < instances.size(); u++) {
                instances[u]->to_graphviz(this, false, instance_text,
                                          all_connections);
                s += "    " + instance_text;
            }
            s += "    label=\"" + host + "\";\n";
            s += "  }\n\n";
        }
    }
    else {
        for (u = 0; u < filter_instances.size(); u++) {
            filter_instances[u]->to_graphviz(this, true, instance_text,
                                             all_connections);
            s += "    " + instance_text;
        }
    }
    for (u = 0; u < all_connections.size(); u++) {
        s += "  " + all_connections[u] + "\n";
    }
    s += "}\n";
    return s;
}

#ifdef DCMPI_HAVE_PCRE
static void remote_run_command_finalize(
    const std::vector<std::string> & hosts,
    std::map<std::string, std::string> & launch_params,
    std::string & run_command,
    std::vector<std::string> & mpi_run_temp_files)
{
    std::string starter;
    starter = "dcmpiremotestartup";
    std::map<std::string, std::string>::iterator it;
    for (it = launch_params.begin();
         it != launch_params.end();
         it++) {
        starter += " " + it->first + " " + it->second;
    }
    for (uint u = 0; u < hosts.size(); u++) {
        starter += " -h " + hosts[u];
    }
    while (1) {
        if (dcmpi_string_replace(run_command, "%e", starter)) {
            ;
        }
        else if (dcmpi_string_replace(run_command, "%F", hosts[0])) {
            ;
        }
        else if (run_command.find("%R") != string::npos) {
            std::string run_file = dcmpi_get_temp_filename();
            mpi_run_temp_files.push_back(run_file);
            FILE * f;
            if ((f = fopen(run_file.c_str(), "w")) == NULL) {
                std::cerr << "ERROR: opening file"
                          << " at " << __FILE__ << ":" << __LINE__
                          << std::endl << std::flush;
                exit(1);
            }
            if (fwrite(starter.c_str(), starter.size(), 1, f) < 1) {
                std::cerr << "ERROR: calling fwrite()"
                          << " at " << __FILE__ << ":" << __LINE__
                          << std::endl << std::flush;
                exit(1);
            }
            if (fwrite("\n", 1, 1, f) < 1) {
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
            dcmpi_string_replace(run_command, "%R", run_file);
        }
        else {
            break;
        }
    }
}
#endif

#ifdef DCMPI_HAVE_PCRE
class RemoteCluster
{
public:
    RemoteCluster(std::string _cluster,
                  std::string _regexp,
                  std::string _startup) :
        cluster(_cluster), regexp(_regexp), startup(_startup), id(-1)
    {
        const char *error;
        int erroffset;
        re = pcre_compile(
            regexp.c_str(),   /* the pattern */
            0,                /* default options */
            &error,           /* for error message */
            &erroffset,       /* for error offset */
            NULL);            /* use default character tables */
        if (!re) {
            std::cerr << "ERROR: compiling regexp " << regexp
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            std::cerr << "ERROR: message from pcre was "
                      << error;
            exit(1);
        }
    }

    bool matches(std::string subject)
    {
        int rc;
        int ovector[30];
        rc = pcre_exec(
            re,             /* result of pcre_compile() */
            NULL,           /* we didn't study the pattern */
            subject.c_str(),/* the subject string */
            subject.size(), /* the length of the subject string */
            0,              /* start at offset 0 in the subject */
            0,              /* default options */
            ovector,        /* vector of integers for substring information */
            30);            /* number of elements in the vector */
        if (rc == 1) {
            return true;
        }
        else {
            return false;
        }
    }

    std::string cluster;
    std::string regexp;
    pcre * re;
    std::string startup;
    int id;
    std::vector<std::string> member_hosts;
};

#endif

void DCLayout::mpi_build_run_commands(
    const std::vector<std::string> & hosts,
    std::map<std::string, std::string> launch_params,
    const std::string & cookie,
    std::vector<std::string> & mpi_run_commands,
    std::vector<std::string> & mpi_run_temp_files)
{
#ifdef DCMPI_HAVE_PCRE
    std::vector<std::string> non_remote_hosts;
    int i, i2;
    uint u, u2;
    std::vector<RemoteCluster*> remote_clusters;
    std::map<std::string, RemoteCluster*> host_remotecluster;
    int remote_cluster_nextid = 1;
#endif
    bool do_multi_execution = false;
    std::vector<std::string> files =get_config_filenames("remote_clusters");
    std::string remote_clusters_filename;
    if (files[0] != "") {
        remote_clusters_filename = files[0];
    }
    if (files[1] != "") {
        remote_clusters_filename = files[1];
    }
    if (dcmpi_file_exists(remote_clusters_filename)) {
#ifndef DCMPI_HAVE_PCRE
        std::cerr << "ERROR:  I observe that you have a "
                  << remote_clusters_filename << " filename, which turns on "
                  << "remote cluster capabilities.  To use remote cluster "
                  << "functionality, you must build dcmpi (at least on the "
                  << "local console side) with the pcre "
                  << "package (see pcre_path build variable).  "
                  << "If you do not wish to use remote cluster functionality, "
                  << "you can simply move the file "
                  << remote_clusters_filename << " out of the way."
                  << std::endl << std::flush;
        exit(1);
#else
        FILE * f;
        if ((f = fopen(remote_clusters_filename.c_str(), "r")) == NULL) {
            std::cerr << "ERROR: opening file"
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
        char line[512];
        std::string cluster;
        std::string regexp;
        std::string startup;
        while (1) {
            cluster ="";
            regexp = "";
            startup = "";
            while (1) { // look for the next cluster name
                if (fgets(line, sizeof(line), f) == NULL) {
                    break;
                }
                line[strlen(line)-1] = 0;
                std::string l = line;
                dcmpi_string_trim(l);
                if ((l[0] == '#') || l.empty()) {
                    ;
                }
                else {
                    cluster = l;
                    break;
                }
            }
            if (cluster.empty()) {
                break;
            }
            while (1) { // read all key/value pairs
                if (fgets(line, sizeof(line), f) == NULL) {
                    break;
                }
                std::string l = line;
                dcmpi_string_trim_rear(l);
                if (l[0] == '#') {
                    ;
                }
                else if (l.empty()) {
                    break;
                }
                else if (l[0] == ' ' && l[1] == ' ') {
                    std::string entry = l.c_str() + 2;
                    dcmpi_string_trim(entry);
                    if (entry.find("matches ") == 0) {
                        std::string val = entry.c_str() + 8;
                        if (val.empty()) {
                            std::cerr << "ERROR:  matches line is empty "
                                      << " for cluster " << cluster
                                      << " in file " << remote_clusters_filename
                                      << " at " << __FILE__ << ":" << __LINE__
                                      << std::endl << std::flush;
                            exit(1);
                        }
                        regexp = val;
                    }
                    else if (entry.find("startup ") == 0) {
                        std::string val = entry.c_str() + 8;
                        if (val.empty()) {
                            std::cerr << "ERROR:  startup line is empty "
                                      << " for cluster " << cluster
                                      << " in file " << remote_clusters_filename
                                      << " at " << __FILE__ << ":" << __LINE__
                                      << std::endl << std::flush;
                            exit(1);
                        }
                        startup = val;
                    }
                }
                else {
                    std::cerr << "ERROR:  bogus line '" << l
                              << "' parsing " << remote_clusters_filename
                              << " at_ " << __FILE__ << ":" << __LINE__
                              << std::endl << std::flush;
                    exit(1);
                }
            }
            remote_clusters.push_back(
                new RemoteCluster(cluster, regexp, startup));
        }

        // see if 'localhost' matches a remote cluster, which is a no-no
        std::string localhost = dcmpi_get_hostname(false);
        std::string localhost_short = dcmpi_get_hostname(true);
        for (u = 0; u < remote_clusters.size(); u++) {
            if (remote_clusters[u]->matches(localhost) ||
                remote_clusters[u]->matches(localhost_short)) {
                std::cerr << "ERROR: the localhost, '" << localhost;
                if (localhost != localhost_short) {
                    std::cerr << "' or '" << localhost_short;
                }
                std::cerr << "' matched as a remote cluster name?  That is "
                          << "invalid.  Please fix your remote_clusters file."
                          << std::endl << std::flush;
                exit(1);
            }
        }


        for (u = 0; u < hosts.size(); u++) {
            const std::string & host = hosts[u];
            bool matched = false;
            for (u2 = 0; u2 < remote_clusters.size(); u2++) {
                if (remote_clusters[u2]->matches(host)) {
                    remote_clusters[u2]->member_hosts.push_back(host);
                    if (remote_clusters[u2]->id == -1) {
                        remote_clusters[u2]->id = remote_cluster_nextid++;
                    }
                    host_remotecluster[host] = remote_clusters[u2];
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                non_remote_hosts.push_back(host);
            }
        }

        if (non_remote_hosts.size() == hosts.size()) {
            ;
        }
        else {
            do_multi_execution = true;
        }
#endif
    }
    std::string mpi_run_command = get_raw_run_command();    
    std::string runtime = get_dcmpiruntime_runtime(mpi_run_command);
    std::string suffix = get_dcmpiruntime_suffix(mpi_run_command);

    if (!do_multi_execution) {
        run_command_finalize(runtime, suffix, hosts, launch_params,
                             mpi_run_command, mpi_run_temp_files);
        mpi_run_commands.push_back(mpi_run_command);
    }
#ifdef DCMPI_HAVE_PCRE
    else {
        int total_clusters = remote_cluster_nextid;

        // tag every filter instance with its cluster ID
        for (u = 0; u < filter_instances.size(); u++) {
            DCFilterInstance * fi = filter_instances[u];
            if (host_remotecluster.count(fi->dcmpi_host)) {
                fi->dcmpi_cluster_rank =
                    host_remotecluster[fi->dcmpi_host]->id;                
            }
        }

        // adjust dcmpi_rank
        int rank_next = 0;
        for (u = 0; u < non_remote_hosts.size(); u++) {
            host_dcmpirank[non_remote_hosts[u]] = rank_next++;
        }
        for (i = 1; i < remote_cluster_nextid; i++) {
            RemoteCluster * rc = NULL;
            // search for the remote cluster with this ID
            for (i2 = 0; i2 < (int)remote_clusters.size(); i2++) {
                if (remote_clusters[i2]->id == i) {
                    rc = remote_clusters[i2];
                    break;
                }
            }
            assert(rc);
            for (u = 0; u < rc->member_hosts.size(); u++) {
                host_dcmpirank[rc->member_hosts[u]] = rank_next++;
            }
        }
        for (u = 0; u < filter_instances.size(); u++) {
            DCFilterInstance * fi = filter_instances[u];
            fi->dcmpi_rank = this->host_dcmpirank[fi->dcmpi_host];
        }

        std::vector<std::string> files = get_config_filenames("remote_proxy");
        std::string remote_proxy_filename;
        if (files[0] != "") {
            remote_proxy_filename = files[0];
        }
        if (files[1] != "") {
            remote_proxy_filename = files[1];
        }

        if (!dcmpi_file_exists(remote_proxy_filename)) {
            std::cerr << "ERROR:  no file "
                      << remote_proxy_filename << " exists.  Please "
                      << "run 'dcmpiconfig' to correct."
                      << std::endl << std::flush;
            exit(1);
        }
        std::string proxy = file_to_string(remote_proxy_filename);
        dcmpi_string_trim(proxy);
        if (!dcmpi_pcre_matches("^([a-zA-Z0-9-]+)(\\.[-a-zA-Z0-9-]+)*:[0-9]+$",
                                proxy)) {
            std::cerr << "ERROR:  invalid syntax '"
                      << proxy << "' in file "
                      << remote_proxy_filename
                      << ".  Please use 'dcmpiconfig' to correct."
                      << std::endl << std::flush;
            exit(1);
        }
        int cluster_offset = 0;
        launch_params["-cluster_rank"] = "0";
        launch_params["-cluster_size"] = dcmpi_to_string(total_clusters);
        launch_params["-cluster_offset"] = "0";
        launch_params["-proxy"] = proxy;

        if (non_remote_hosts.size() == 0) {
            std::cerr << "ERROR: all hosts are remote, including localhost?  "
                      << "Something is probably wrong with your "
                      << "remote_clusters file."
                      << std::endl << std::flush;
            exit(1);
        }

        run_command_finalize(
            runtime, suffix, non_remote_hosts,
            launch_params, mpi_run_command, mpi_run_temp_files);
        mpi_run_commands.push_back(mpi_run_command);

        launch_params.erase("-console");
        cluster_offset += non_remote_hosts.size();
        for (i = 1; i < remote_cluster_nextid; i++) {
            RemoteCluster * rc = NULL;
            // search for the remote cluster with this ID
            for (i2 = 0; i2 < (int)remote_clusters.size(); i2++) {
                if (remote_clusters[i2]->id == i) {
                    rc = remote_clusters[i2];
                    break;
                }
            }
            assert(rc);
            launch_params["-dcmpi_rank"] = dcmpi_to_string(cluster_offset);
            launch_params["-cluster_rank"] = dcmpi_to_string(i);
            launch_params["-cluster_size"] = dcmpi_to_string(total_clusters);
            launch_params["-cluster_offset"] = dcmpi_to_string(cluster_offset);
            cluster_offset += rc->member_hosts.size();
            std::string executable;
            if (rc->startup == "") {
                std::cerr << "ERROR: startup command must be specified "
                          << " at " << __FILE__ << ":" << __LINE__
                          << std::endl << std::flush;
                exit(1);
            }
            std::string run_command = rc->startup;
            remote_run_command_finalize(
                rc->member_hosts, launch_params,
                run_command, mpi_run_temp_files);
            mpi_run_commands.push_back(run_command);
        }
    }
    for (u = 0; u < remote_clusters.size(); u++) {
        delete remote_clusters[u];
    }
#endif
}

void DCLayout::add(DCFilterInstance * filter_instance)
{
    uint u;
    for (u = 0; u < filter_instances.size(); u++) {
        if (filter_instances[u] == filter_instance) {
            std::cerr << "ERROR: in DCLayout::add():  "
                      << "filter instance "
                      << filter_instance
                      << " (" << filter_instance->get_distinguished_name()
                      << ") already in layout!" << std::endl;
            exit(1);
        }
        else if (filter_instances[u]->get_distinguished_name() ==
                 filter_instance->get_distinguished_name()) {
            std::cerr << "ERROR: in DCLayout::add():  "
                      << "there is already a filter instance with "
                      << "distinguished name "
                      << filter_instance->get_distinguished_name()
                      << " in layout!" << std::endl;
            exit(1);
        }
    }
    this->filter_instances.push_back(filter_instance);
}

void DCLayout::add(DCFilterInstance & filter_instance)
{
    this->add(&filter_instance);
}

void DCLayout::add_port(DCFilterInstance * filter1, std::string port1,
                        DCFilterInstance * filter2, std::string port2)
{
    if (port1.size() > DCMPI_PORT_MAXLEN_SIZE) {
        std::cerr << "ERROR: port" << port1 << " must be <= "
                  << DCMPI_PORT_MAXLEN_SIZE << " characters long"
                  << std::endl << std::flush;
        exit(1);
    }
    if (port2.size() > DCMPI_PORT_MAXLEN_SIZE) {
        std::cerr << "ERROR: port" << port2 << " must be <= "
                  << DCMPI_PORT_MAXLEN_SIZE << " characters long"            
                  << std::endl << std::flush;
        exit(1);
    }
    if (port1.size() == 0) {
        std::cerr << "ERROR: port1 is empty"
                  << std::endl << std::flush;
        assert(0);
    }
    if (port2.size() == 0) {
        std::cerr << "ERROR: port2 is empty"
                  << std::endl << std::flush;
        assert(0);
    }
    if (!filter1) {
        std::cerr << "ERROR: filter1 is NULL"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        assert(0);
    }
    if (!filter2) {
        std::cerr << "ERROR: filter2 is NULL"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        assert(0);
    }
    filter1->add_port(port1, filter2, port2);
}

void DCLayout::add_port(DCFilterInstance & filter1, std::string port1,
                        DCFilterInstance & filter2, std::string port2)
{
    this->add_port(&filter1, port1, &filter2, port2);
}

void DCLayout::add_propagated_environment_variable(
    const char * varname,
    bool override_if_present)
{
    if (getenv(varname)) {
        propagated_environment_variables[varname] = override_if_present;
    }
}

void DCLayout::serialize(DCBuffer * buf) const
{
    uint u;
    int4 len;

    // send the propagated environment variables over, but they won't end up
    // in the layout
    len = propagated_environment_variables.size();
    buf->Append(len);
    std::map<std::string,bool>::const_iterator it;
    for (it = propagated_environment_variables.begin();
         it != propagated_environment_variables.end();
         it++) {
        const std::string & k = it->first;
        std::string v = getenv(k.c_str());
        buf->Append(k);
        buf->Append(v);
        buf->pack("i", (int4)(it->second?0:1));
    }

    len = filter_instances.size();
    buf->Append(len);
    for (u = 0; u < filter_instances.size(); u++)
    {
        filter_instances[u]->serialize(buf);
    }
    buf->Append((uint1)(init_filter_broadcast?1:0));
    if (init_filter_broadcast)
        init_filter_broadcast->saveToDCBuffer(buf);

    buf->Append((uint1)(has_console_filter?1:0));

    filter_reg->serialize(buf);
}

void DCLayout::deSerialize(DCBuffer * buf)
{
    int i;
    int4 len;
    uint1 has_init_filter_broadcast;
    std::string s;

    buf->Extract(&len);
    for (i = 0; i < len; i++) {
        std::string k, v;
        int4 override;
        buf->Extract(&k);
        buf->Extract(&v);
        buf->Extract(&override);
        if (dcmpi_verbose()) {
            cerr << "on " << localhost_shortname << " setenv(\"" << k << "\",\""
                 << v << "\")" << endl;
        }
        setenv(k.c_str(), v.c_str(), override);
    }

    buf->Extract(&len);
    for (i = 0; i < len; i++) {
        DCFilterInstance * fi = new DCFilterInstance();
        fi->deSerialize(buf);
        filter_instances.push_back(fi);
        deserialized_filter_instances.push_back(fi);
    }
    buf->Extract(&has_init_filter_broadcast);
    if (has_init_filter_broadcast) {
        init_filter_broadcast = new DCBuffer;
        init_filter_broadcast->restoreFromDCBuffer(buf);
    }
    else {
        init_filter_broadcast = NULL;
    }

    uint1 hcf;
    buf->Extract(&hcf);
    has_console_filter = (hcf==1);

    filter_reg->deSerialize(buf);
}

void DCLayout::expand_transparents()
{
    while (1)
    {
        bool expanded = false;
        std::vector<DCFilterInstance*> new_copies;
        uint u;
        for (u = 0; u < this->filter_instances.size(); u++) {
            DCFilterInstance * fi = filter_instances[u];
            if ((fi->num_copies > 1) &&
                (fi->transparent_copies_expanded == false)) {
                if (dcmpi_verbose()) {
                    cout << "transparent-copy expanding "
                         << fi->get_distinguished_name() << endl;
                }
                expanded = true;
                fi->transparent_copies_expanded = true;
                // make sure bind_hosts info is propagated around to
                // copies
                std::vector<std::string> bind_hosts_list;
                if (fi->bind_hosts.size() > 1) {
                    assert((int)(fi->bind_hosts.size()) == fi->num_copies);
                    bind_hosts_list = fi->bind_hosts;

                    std::vector<std::string> bhls;
                    bhls.push_back(bind_hosts_list[0]);
                    fi->bind_hosts = bhls;
                    bind_hosts_list.erase(bind_hosts_list.begin());
                }

                std::vector<std::vector<std::string> > remaining_labels;
                if (!fi->layout_labels.empty()) {
                    remaining_labels = fi->layout_labels;
                    std::vector<std::vector<std::string> > me;
                    me.push_back(remaining_labels[0]);
                    fi->layout_labels = me;
                    remaining_labels.erase(remaining_labels.begin());
                }

                int n = 1;
                while (n < fi->num_copies) {
                    DCFilterInstance * newval =
                        new DCFilterInstance(fi->filtername, fi->instancename);
                    *newval = *fi; // this is a shallow copy b/c there's no
                                   // copy constructor defined to do otherwise
                    newval->copy_rank = n;
                    expansion_filter_instances.push_back(newval);
                    new_copies.push_back(newval);
                    n += 1;
                    if (!bind_hosts_list.empty()) {
                        std::vector<std::string> bhls;
                        bhls.push_back(bind_hosts_list[0]);
                        newval->bind_hosts = bhls;
                        bind_hosts_list.erase(bind_hosts_list.begin());
                    }
                    if (!remaining_labels.empty()) {
                        std::vector<std::vector<std::string> > me;
                        me.push_back(remaining_labels[0]);
                        newval->layout_labels = me;
                        remaining_labels.erase(remaining_labels.begin());
                    }
                }
                // expand any outports that point to the expanding
                // filter
                std::vector<MultiPort*> new_mappings;
                std::vector<DCFilterInstance*> new_vals;
                std::vector<std::string> new_ports;

                uint u2;
                for (u2 = 0; u2 < filter_instances.size(); u2++) {
                    DCFilterInstance * i = filter_instances[u2];
                    std::map<std::string, MultiPort>::iterator it;
                    for (it = i->outports.begin();
                         it != i->outports.end();
                         it++) {
                        MultiPort * mp = &(it->second);
                        uint u3;
                        for (u3 = 0; u3 < mp->remotes.size(); u3++) {
                            Instance_Port rp = mp->remotes[u3];
                            if (rp.instance == fi) {
                                // this port points at the expanded
                                // thing, so add new parts to this
                                // port
                                uint u4;
                                for (u4 = 0;
                                     u4 < new_copies.size();
                                     u4++) {
                                    new_mappings.push_back(mp);
                                    new_vals.push_back(new_copies[u4]);
                                    new_ports.push_back(rp.port);
                                }
                            }
                        }
                    }
                }

                for (u2 = 0; u2 < new_mappings.size(); u2++) {
                    new_mappings[u2]->add_remote(new_vals[u2],
                                                 new_ports[u2]);
                }
                break;
            }
        }
        if (!expanded)
            break;
        else {
            uint nci;
            for (nci = 0; nci < new_copies.size(); nci++) {
                // insert right after the expanded one
                this->filter_instances.insert(
                    std::find(filter_instances.begin(),
                              filter_instances.end(),
                              filter_instances[u]) + nci,
                    new_copies[nci]);
            }
        }
    }
}

void DCLayout::expand_composites()
{

    while (1)
    {
        bool did_composite = false;
        std::vector<DCFilterInstance*> new_instance_members;
        DCFilterInstance * drop_instance = NULL;
        uint u;
        for (u = 0; u < filter_instances.size(); u++) {
            DCFilterInstance * expanded = filter_instances[u];
            DCFilter * f = filter_reg->get_filter(expanded->filtername);
            if (!f) {
                std::cerr << "ERROR: cannot get filter named '"
                          << expanded->filtername << "', perhaps your\n"
                          << "dcmpi_provideXXX(...)\n"
                          << "call is not present in the file(s) which "
                          << "produce(s) the library\n"
                          << filter_reg->filter_to_libfile[
                              expanded->filtername] << endl;
                exit(1);
            }
            DCLayout * new_layout = f->get_composite_layout(expanded->composite_cluster_binding);
            delete f;
            if (new_layout) { // composite layout

                did_composite = true;

                // validate that there at most 1 inbound designate and 1
                // outbound designate
                int num_inbounds = 0;
                int num_outbounds = 0;
                uint u2;
                for (u2=0; u2 < new_layout->filter_instances.size(); u2++) {
                    DCFilterInstance * nli =
                        new_layout->filter_instances[u2];
                    if (nli->is_inbound_composite_designate) {
                        num_inbounds += 1;
                    }
                    if (nli->is_outbound_composite_designate) {
                        num_outbounds += 1;
                    }
                }
                if (num_inbounds != 1) {
                    std::cerr << "ERROR: more or less than 1 inbound "
                              <<"designate exists for composite "
                              << expanded->filtername << std::endl;
                    exit(1);
                }
                if (num_outbounds != 1) {
                    std::cerr << "ERROR: more or less than 1 outbound "
                              <<"designate exists for composite "
                              << expanded->filtername << std::endl;
                    exit(1);
                }
                new_instance_members = new_layout->filter_instances;
                uint u3;
                for (u3 = 0; u3 < new_instance_members.size(); u3++) {
                    // inherit a few values from the containing
                    // composite filter
                    DCFilterInstance * ni = new_instance_members[u3];
                    ni->heritage = expanded->get_distinguished_name();

                    // allow deeper nested filter to override
                    // bind_hosts of outer filter
                    if (expanded->bind_hosts.size() && ni->bind_hosts.size())
                    {
                        ;
                    }
                    else if (expanded->bind_hosts.size()) {
                        ni->bind_hosts = expanded->bind_hosts;
                    }
                }
                drop_instance = expanded;

                for (u3 = 0; u3<new_layout->filter_instances.size(); u3++) {
                    DCFilterInstance * i =
                        new_layout->filter_instances[u3];
                    // figure out who is pointing at the designated in and
                    // update its pointer
                    if (i->is_inbound_composite_designate) {
                        DCFilterInstance * inbound_designate = i;
                        uint u4;
                        for (u4 = 0; u4 < this->filter_instances.size(); u4++) {
                            DCFilterInstance * j =
                                this->filter_instances[u4];
                            std::map<std::string, MultiPort>::iterator it;
                            for (it = j->outports.begin();
                                 it != j->outports.end();
                                 it++)
                            {
                                MultiPort & v = (it->second);
                                std::vector<Instance_Port>::iterator it2;
                                for (it2 = v.remotes.begin();
                                     it2 != v.remotes.end();
                                     it2++) {
                                    Instance_Port & rpi = *it2;
                                    if (rpi.instance == expanded) {
                                        rpi.instance = inbound_designate;
                                    }
                                }
                            }
                        }
                        break;
                    }
                }

                // figure out who, if any, in this composite filter, is
                // pointing at something external, and link it up
                for (u3 = 0; u3<new_layout->filter_instances.size(); u3++) {
                    DCFilterInstance * nli =
                        new_layout->filter_instances[u3];
                    if (nli->is_outbound_composite_designate &&
                        expanded->outports.size() > 0) {
                        assert((nli->outports.size()) == 0);
                        std::map<std::string, MultiPort>::iterator it;
                        for (it = expanded->outports.begin();
                             it != expanded->outports.end();
                             it++) {
                            std::string pn = it->first;
                            MultiPort & mp = (it->second);
                            std::vector<Instance_Port>::iterator it2;
                            for (it2 = mp.remotes.begin();
                                 it2 != mp.remotes.end();
                                 it2++) {
                                nli->add_port(
                                    pn, it2->instance, it2->port);
                            }
                        }
                        break;
                    }
                }
                delete new_layout;
                // need to look again at everything after this expansion, so
                // break, add the new expansion items, drop the expanded item,
                // and start the loop again
                break;
            }
        }
        if (did_composite) {
            for (u = 0; u < new_instance_members.size(); u++) {
                DCFilterInstance * expansion = new_instance_members[u];
                this->filter_instances.push_back(expansion);
                this->expansion_filter_instances.push_back(expansion);
            }
            std::vector<DCFilterInstance*>::iterator it =
                std::find(filter_instances.begin(),
                          filter_instances.end(),
                          drop_instance);
            assert(it != filter_instances.end());
            this->filter_instances.erase(it);
        }
        else {
            break;
        }
    }
}

void DCLayout::do_placement()
{
    std::string console_name = "<console>";
    std::set<std::string> placed_hosts_from_pool;
    std::set<std::string> placed_hosts_not_in_pool;
    int gftid = 0;
    uint u;
    if (exec_host_pool.empty()) {
        for (u = 0; u < filter_instances.size(); u++) {
            DCFilterInstance * fi = filter_instances[u];
            if ((fi->bind_hosts.size()== 0) &&
                (fi->filtername != console_name)) {
                std::cerr << "ERROR: nowhere to place filter "
                          << fi->get_distinguished_name()
                          << " to, host pool is empty.  Consider binding"
                          << " the filter to a host with the"
                          << " DCFilterInstance::bind_to_host() method."
                          << std::endl << std::flush;
                exit(1);
            }
        }
    }
    Ring<std::string> placeable_hosts_ring(exec_host_pool);

    std::map< std::string, std::map<std::string, int> >
        host_to_placed_filter_counts;
    // bind to hosts, don't assign rank yet
    for (u = 0; u < filter_instances.size(); u++) {
        DCFilterInstance * fi = filter_instances[u];
        std::string bh;
        // at this point every bind_hosts list should be of size 0 or 1
        if ((fi->bind_hosts.size() > 0) && (fi->bind_hosts[0].empty()==false)) {
            assert(fi->bind_hosts.size() == 1);
            bh = fi->bind_hosts[0];
            placed_hosts_not_in_pool.insert(bh);
        }
        else if (fi->filtername == console_name) {
            bh = console_name; // just a symbolic name for display purposes
        }
        else { // just a normal filter
            if (placeable_hosts_ring.size() == 0) {
                std::cerr << "ERROR:  trying to place an unbound filter "
                          << "without a host pool present"
                          << " at " << __FILE__ << ":" << __LINE__
                          << std::endl << std::flush;
                exit(1);
            }
            bh = placeable_hosts_ring.next();
            placed_hosts_from_pool.insert(bh);
        }
        fi->given_bind_host = bh;
        if (bh != console_name && resembles_localhost(bh)) {
            bh = localhost;
        }
        fi->dcmpi_host = bh;
        fi->gftid = gftid;
        gftid += 1;

        // assign runtime label
        if (fi->layout_labels.size() > 0) {
            assert(fi->layout_labels.size() == 1);
            fi->runtime_labels = fi->layout_labels[0];
        }

        // assign local copy ranks
        std::string unique_name = fi->get_distinguished_name();
        if(host_to_placed_filter_counts.count(bh)== 0) {
            host_to_placed_filter_counts[bh] = std::map<std::string, int>();
        }
        if (host_to_placed_filter_counts[bh].count(unique_name) == 0) {
            host_to_placed_filter_counts[bh][unique_name] = 0;
        }
        fi->local_copy_rank = host_to_placed_filter_counts[bh][unique_name];
        host_to_placed_filter_counts[bh][unique_name]++;
    }

    // assign local max copy ranks
    for (u = 0; u < filter_instances.size(); u++) {
        DCFilterInstance * fi = filter_instances[u];
        int num_this_filter_this_host =
            host_to_placed_filter_counts[fi->dcmpi_host]
            [fi->get_distinguished_name()];
        fi->local_num_copies = num_this_filter_this_host;
    }

#define DCMPI_REQUIRE_LOCALHOST

    std::set<std::string> run_hosts_set;
    assert(run_hosts.empty());
#ifdef DCMPI_REQUIRE_LOCALHOST
    run_hosts.push_back(localhost);
    run_hosts_set.insert(localhost);
#else
    bool used_localhost = false;
#endif
    // assemble all actual runtime hosts in a vector with the index-1 into the
    // array being the dcmpi rank; localhost is always at rank 0 and <console>
    // is at rank -1, so localhost is at pos 1 and <console> at pos 0
    std::set<std::string>::iterator it8;
    for (it8 = placed_hosts_from_pool.begin();
         it8 != placed_hosts_from_pool.end();
         it8++) {
        if (!resembles_localhost(*it8)) {
            run_hosts.push_back(*it8);
            run_hosts_set.insert(*it8);
        }
#ifndef DCMPI_REQUIRE_LOCALHOST
        else if (!used_localhost) {
            run_hosts.push_back(localhost);
            run_hosts_set.insert(localhost);
            used_localhost = true;
        }
#endif
    }
    std::set<std::string>::iterator it;
    for (it = placed_hosts_not_in_pool.begin();
         it != placed_hosts_not_in_pool.end();
         it++) {
        const std::string & hn = *it;
        if (run_hosts_set.find(hn)==run_hosts_set.end() &&
            !resembles_localhost(hn)) {
            run_hosts.push_back(hn);
            run_hosts_set.insert(hn);
        }
    }
#ifdef DEBUG
    if (run_hosts_set.size() != run_hosts.size()) {
        std::cerr << "ERROR: run_hosts_set.size() != run_hosts.size()"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        std::copy(run_hosts_set.begin(), run_hosts_set.end(), ostream_iterator<string>(cout, " "));
        std::cout << endl;
        std::copy(run_hosts.begin(), run_hosts.end(), ostream_iterator<string>(cout, " "));
        std::cout << endl;
        assert(0);
    }
#endif
    run_hosts_no_console = run_hosts;

    run_hosts.insert(run_hosts.begin(), console_name);

    // now, assign dcmpi_rank based off of their array positions
    for (u = 0; u < run_hosts.size(); u++) {
        host_dcmpirank[run_hosts[u]] = u - 1;
    }
    for (u = 0; u < filter_instances.size(); u++) {
        DCFilterInstance * fi = filter_instances[u];
        fi->dcmpi_rank = this->host_dcmpirank[fi->dcmpi_host];
    }

    // by now every filter_instance has a gftid, so resolve the
    // inports and outports that way
    for (u = 0; u < filter_instances.size(); u++) {
        DCFilterInstance * fi = filter_instances[u];
        std::map<std::string, MultiPort>::iterator it;
        for (it = fi->outports.begin();
             it != fi->outports.end();
             it++) {
            std::string pn = it->first;
            MultiPort & mp = it->second;
            ResolvedMultiPort new_resolved_port;
            uint u2;
            for (u2 = 0; u2 < mp.remotes.size(); u2++) {
                Instance_Port & ip = mp.remotes[u2];
                new_resolved_port.add_remote(ip.instance->gftid, ip.port);
                ip.instance->resolved_inports[ip.port].insert(
                    Gftid_Port(fi->gftid, pn));
            }
            fi->resolved_outports[pn] = new_resolved_port;
        }
    }
}

void DCLayout::show_graphviz()
{
    if (!do_graphviz) {
        return;
    }
    std::string cmd;
    std::string s = this->to_graphviz();
    std::string tempfn = dcmpi_get_temp_filename();
    FILE * f = fopen(tempfn.c_str(), "w");
    fwrite(s.c_str(), s.length(), 1, f);
    fclose(f);
    if (dcmpi_verbose()) {
        cout << s;
    }

    if (dcmpi_string_ends_with(graphviz_output, ".ps")) {
        cmd = "dot -Tps -o ";
        cmd += graphviz_output;
        cmd += " ";
        cmd += tempfn;
    }
    else if (dcmpi_string_ends_with(graphviz_output, ".dot")) {
        cmd = "cp ";
        cmd += tempfn;        
        cmd += " ";
        cmd += graphviz_output;
    }
    else {
        std::cerr << "ERROR: invalid graphviz output file type "
                  << graphviz_output
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }

    if (dcmpi_verbose()) {
        cout << cmd << endl;
    }
    system(cmd.c_str());
    remove(tempfn.c_str());

    if (graphviz_output_posthook.size()) {
        if (dcmpi_verbose()) {
            cout << graphviz_output_posthook << endl;
        }
        system(graphviz_output_posthook.c_str());
    }
}

void DCLayout::print_global_thread_ids_table()
{
    printf("GFTID DCMR CR HOST       FILTER_INSTANCE                LABELS\n");
    printf("----- ---- -- ----       ---------------                ------\n");
    int n = 0;
    while (n < (int)gftids_to_instances.size()) {
        DCFilterInstance * fi = gftids_to_instances[n];
        printf("%-5d %-4d %-2d %-10s %-30s", n,
               fi->dcmpi_rank, fi->dcmpi_cluster_rank,
               fi->dcmpi_host.c_str(), fi->get_distinguished_name().c_str());
        const std::vector<std::string> & runtime_labels = fi->runtime_labels;
        if (runtime_labels.empty() == false) {
            for (uint u = 0; u < runtime_labels.size(); u++) {
                if (u) {
                    cout << ",";
                }
                cout << runtime_labels[u];
            }
        }
        cout << endl;
        n += 1;
    }

    printf("OUTPORTS TABLE\n");
    n = 0;
    while (n < (int)gftids_to_instances.size()) {
        printf("    GFTID %d\n", n);
        DCFilterInstance * inst = gftids_to_instances[n];
        std::map<std::string, ResolvedMultiPort>::iterator it;
        for (it = inst->resolved_outports.begin();
             it != inst->resolved_outports.end();
             it++) {
            const std::string & pn = it->first;
            const ResolvedMultiPort & rmp = it->second;
            uint u2 = 0;
            for (u2 = 0; u2 < rmp.remotes.size(); u2++) {
                printf("        Port %s -> %d[%s]\n",
                       pn.c_str(), rmp.remotes[u2].gftid,
                       rmp.remotes[u2].port.c_str());
            }
        }
        n += 1;
    }

    printf("INPORTS TABLE\n");
    n = 0;
    while (n < (int)gftids_to_instances.size()) {
        printf("    GFTID %d\n", n);
        DCFilterInstance * inst = gftids_to_instances[n];
        std::map<std::string, std::set<Gftid_Port> >::iterator it;
        for (it = inst->resolved_inports.begin();
             it != inst->resolved_inports.end();
             it++) {
            std::string pn = it->first;
            std::set<Gftid_Port> & set_of_inports = it->second;
            std::set<Gftid_Port>::iterator it2;
            for (it2 = set_of_inports.begin();
                 it2 != set_of_inports.end();
                 it2++) {
                printf("        Port %s receives from %d[%s]\n",
                       pn.c_str(), it2->gftid, it2->port.c_str());
            }
        }
        n += 1;
    }
}

void DCLayout::set_param_all(std::string key, std::string value)
{
    set_param_all_params[key] = value;
}

void DCLayout::set_param_all(std::string key, int value)
{
    this->set_param_all(key, dcmpi_to_string(value));
}

// expand filters phase
void DCLayout::execute1()
{
    // ensure that all filters that are involved in ports are filters in the
    // set of instances
    uint u;
    for (u = 0; u < filter_instances.size(); u++) {
        DCFilterInstance * fi = filter_instances[u];
        std::map<std::string, MultiPort>::iterator it = fi->outports.begin();
        while (it != fi->outports.end()) {
            const MultiPort & mp = it->second;
            for (uint u2 = 0; u2 < mp.remotes.size(); u2++) {
                const Instance_Port & ip = mp.remotes[u2];
                if (std::find(filter_instances.begin(),
                              filter_instances.end(),
                              ip.instance) == filter_instances.end()) {
                    std::cerr << "ERROR: filter " << ip.instance->filtername
                              << " (instance name " << ip.instance->instancename
                              << ", pointer " << ip.instance
                              << ") does not exist in layout,\n"
                              << "    but is mentioned in a port connection.  "
                              << "Did you forget to add the\n"
                              << "    filter instance to the layout?"
                              << std::endl << std::flush;
                    exit(1);
                }
            }
            it++;
        }
    }

    // set "all params"
    std::map<std::string,std::string>::iterator itall;
    for (itall = set_param_all_params.begin();
         itall != set_param_all_params.end(); itall++) {
        for (u = 0; u < filter_instances.size(); u++) {
            DCFilterInstance * fi = filter_instances[u];
            fi->set_param(itall->first, itall->second);
        }
    }
    
    // determine if we'll do graphviz
    std::string gfile = dcmpi_get_home();
    gfile += "/graphviz";
    do_graphviz = false;
    if (fileExists(gfile)) {
        std::map<std::string, std::string> pairs = file_to_pairs(gfile.c_str());
        if ((pairs.count("enabled")) &&
            (pairs["enabled"] == "yes") &&
            (pairs.count("output"))) {
            do_graphviz = true;
            graphviz_output = pairs["output"];
            if (pairs.count("posthook")) {
                graphviz_output_posthook = pairs["posthook"];
                dcmpi_string_replace(graphviz_output_posthook,
                                     "%o",
                                     graphviz_output);
            }
        }
    }

#ifdef DEBUG3
    if (dcmpi_verbose()) {
        cout << "basic layout:\n";
        this->show_graphviz();
    }
#endif
    this->expand_transparents();
#ifdef DEBUG3
    if (dcmpi_verbose()) {
        cout << "after transparent expansion(1):\n";
        this->show_graphviz();
    }
#endif
    this->expand_composites();
#ifdef DEBUG3
    if (dcmpi_verbose()) {
        cout << "after composite expansion:\n";
        this->show_graphviz();
    }
#endif
    this->expand_transparents();
#ifdef DEBUG3
    if (dcmpi_verbose()) {
        cout << "after transparent expansion(2):\n";
        this->show_graphviz();
    }
#endif
    this->do_placement();
    this->show_graphviz();

    // build map from gftids to instances to be used during execution on the
    // local side
    gftids_to_instances.clear();
    for (u = 0; u < filter_instances.size(); u++) {
        DCFilterInstance * fi = filter_instances[u];
        gftids_to_instances[fi->gftid] = fi;
    }
}

// init runtime phase
void DCLayout::execute2()
{
    uint u;
    console_to_mpi_queue = new Queue<DCMPIPacket*>;
    
    has_console_filter = false;
    console_instance = NULL;
    console_exe = NULL;
    for (u = 0; u < filter_instances.size(); u++) {
        DCFilterInstance * fi = filter_instances[u];
        if (fi->filtername == "<console>") {
            has_console_filter = true;
            console_instance = fi;
            break;
        }
    }

    std::string console_config = dcmpi_get_home() + "/console";
    if (!fileExists(console_config)) {
        uses_unix_socket_console_bridge = true; // default to UNIX socket
    }
    else {
        std::map<std::string,std::string> pairs = file_to_pairs(console_config);
        if (pairs.count("socket_type")) {
            if (pairs["socket_type"] == "unix") {
                uses_unix_socket_console_bridge = true;
            }
            else if (pairs["socket_type"] == "tcp") {
                uses_unix_socket_console_bridge = false;
            }
            else {
                std::cerr << "ERROR: in " << console_config
                          << " (use 'socket_type unix' or 'socket_type tcp')"
                          << std::endl << std::flush;
                exit(1);
            }
        }
        else {
            uses_unix_socket_console_bridge = false;
        }
    }
    std::string to_info;
    std::string from_info;
    std::string type_info;
    int to_listen;
    int from_listen;
    if (uses_unix_socket_console_bridge) {
        unix_socket_tempd = dcmpi_get_temp_dir();
        consoleToMPITempFn = unix_socket_tempd + "/console_out";
        consoleFromMPITempFn = unix_socket_tempd + "/console_in";

        to_listen = dcmpiOpenUnixListenSocket(consoleToMPITempFn.c_str());
        if (to_listen == -1) {
            std::cerr << "ERROR: opening unix listen socket"
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
        from_listen = dcmpiOpenUnixListenSocket(consoleFromMPITempFn.c_str());
        if (from_listen == -1) {
            std::cerr << "ERROR: opening unix listen socket"
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
        type_info = "unix";
        to_info = consoleToMPITempFn;
        from_info = consoleFromMPITempFn;
    }
    else {
        uint2 to_port;
        uint2 from_port;
        to_listen = dcmpiOpenListenSocket(&to_port);
        if (!to_listen) {
            std::cerr << "ERROR: opening tcp listen socket"
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
        from_listen = dcmpiOpenListenSocket(&from_port);
        if (!from_listen) {
            std::cerr << "ERROR: opening tcp listen socket"
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
        type_info = "tcp";
        std::vector<std::string> ips = dcmpi_get_ip_addrs();
        to_info = "";
        for (uint i = 0; i < ips.size(); i++) {
            if (i) {
                to_info+= ',';
            }
            to_info+= ips[i];
        }
        from_info = to_info;
        to_info += "/" + dcmpi_to_string(to_port);
        from_info += "/" + dcmpi_to_string(from_port);
    }

    // generate cookie (globally unique ID)
    double t = dcmpi_doubletime();
    int seconds = (int)t;
    int frac_seconds = (int)((t - seconds) * 1000000);
    std::string cookie = dcmpi_get_hostname() + "_" +
        dcmpi_to_string(seconds) + "." + dcmpi_to_string(frac_seconds) + "_" +
        dcmpi_to_string(getpid()) + "_" + dcmpi_to_string(dcmpi_rand());

    std::map<std::string, std::string> launch_params;
    launch_params["-dcmpi_rank"] = "0";
    launch_params["-cluster_rank"] = "0";
    launch_params["-cluster_size"] = "1";
    launch_params["-cluster_offset"] = "0";
    launch_params["-console"] = type_info + ":" + to_info + ":" + from_info;
    launch_params["-cookie"] = cookie;
    launch_params["-totaldcmpiranks"] =
        dcmpi_to_string(run_hosts_no_console.size() + (has_console_filter?1:0));
    launch_params["-hasconsolefilter"] = has_console_filter?"yes":"no";

    std::vector<std::string> mpi_run_commands;
    mpi_build_run_commands(run_hosts_no_console, launch_params, cookie,
                           mpi_run_commands, mpi_run_temp_files);

    // potentially allocate and setup the console filter
    if (has_console_filter) {
        uint u2;
        console_filter = new DCFilter;
        console_exe = new DCFilterExecutor;
        console_exe->setup(console_instance, console_filter);
        console_filter->executor = console_exe;

        // set up console filter's outports
        std::map<std::string, ResolvedMultiPort>::iterator it2;
        for (it2 = console_instance->resolved_outports.begin();
             it2 != console_instance->resolved_outports.end();
             it2++) {
            const std::string & multiportKey = it2->first;
            ResolvedMultiPort & rmp = it2->second;
            for (u2 = 0; u2 < rmp.remotes.size(); u2++) {
                Gftid_Port & gp = rmp.remotes[u2];
                int remote_rank = gftids_to_instances[gp.gftid]->dcmpi_rank;
                int remote_cluster_rank = gftids_to_instances[gp.gftid]->dcmpi_cluster_rank;
                QueueWriter * qw;
                qw = new RemoteQueueWriter(
                    remote_rank, remote_cluster_rank,
                    gp.gftid, gp.port,
                    console_instance->dcmpi_rank, 0 /* console is cluster 0 */,
                    console_instance->gftid, multiportKey,
                    console_to_mpi_queue, gftids_to_instances[gp.gftid]);
                if (console_exe->out_ports.count(multiportKey) == 0) {
                    console_exe->out_ports[multiportKey] =
                        new MultiOutportManager(
                            DCMPI_MULTIOUT_POLICY_ROUND_ROBIN,
                            console_instance);
                }
                console_exe->out_ports[multiportKey]->add_target(qw);
            }
            console_exe->out_ports[multiportKey]->note_final_target();
        }

        ranks_console_sends_to.clear();
        std::map<std::string, ResolvedMultiPort>::iterator it3;
        for (it3 = console_instance->resolved_outports.begin();
             it3 != console_instance->resolved_outports.end();
             it3++) {
            ResolvedMultiPort & rmp = it3->second;
            for (uint u2 = 0; u2 < rmp.remotes.size(); u2++) {
                Gftid_Port & gp = rmp.remotes[u2];
                int remote_rank = gftids_to_instances[gp.gftid]->dcmpi_rank;
                ranks_console_sends_to.insert(remote_rank);
            }
        }
        assert(ranks_console_sends_to.count(-1) == 0);

        ranks_console_hears_from.clear();
        for (uint u = 0; u < filter_instances.size(); u++) {
            DCFilterInstance * fi = filter_instances[u];
            if (fi->filtername != "<console>") {
                std::map<std::string, ResolvedMultiPort>::iterator it2;
                for (it2 = fi->resolved_outports.begin();
                     it2 != fi->resolved_outports.end();
                     it2++) {
                    ResolvedMultiPort & rmp = it2->second;
                    for (uint u2 = 0; u2 < rmp.remotes.size(); u2++) {
                        Gftid_Port & gp = rmp.remotes[u2];
                        int remote_rank = gftids_to_instances[gp.gftid]->dcmpi_rank;
                        if (remote_rank == -1) {
                            ranks_console_hears_from.insert(fi->dcmpi_rank);
                        }
                    }
                }
            }
        }
        assert(ranks_console_hears_from.count(-1) == 0);

        if (dcmpi_verbose()) {
            cout << "other ranks console sends to: ";
            copy(ranks_console_sends_to.begin(), ranks_console_sends_to.end(), ostream_iterator<int>(cout, " "));
            cout << endl;
            cout << "other ranks that send to console: ";
            copy(ranks_console_hears_from.begin(), ranks_console_hears_from.end(), ostream_iterator<int>(cout, " "));
            cout << endl;
        }
    }

    console_to_mpi_thread =
        new ConsoleToMPIThread(uses_unix_socket_console_bridge,
                               to_listen, console_to_mpi_queue, this);
    console_from_mpi_thread =
        new ConsoleFromMPIThread(uses_unix_socket_console_bridge,
                                 from_listen, console_exe, this);
    console_to_mpi_thread->start();
    console_from_mpi_thread->start();

    if (dcmpi_verbose()) {
        // this uses gftids_to_instances, so must be called after it's built
        this->print_global_thread_ids_table();
    }

    for (u = 0; u < mpi_run_commands.size(); u++) {
        if (dcmpi_verbose()) {
            cout << "running " << mpi_run_commands[u] << endl;
        }
        mpi_command_threads.push_back(new DCCommandThread(mpi_run_commands[u]));
        mpi_command_threads[u]->start();
    }

    DCBuffer * serializedLayout = new DCBuffer();
    this->serialize(serializedLayout);
    DCMPIPacketAddress addr(-1, -1, -1, "", -1, -1, -1, "");
    DCMPIPacket * layoutp = new DCMPIPacket(
        addr, DCMPI_PACKET_TYPE_LAYOUT,
        serializedLayout->getUsedSize(), serializedLayout);
    console_to_mpi_queue->put(layoutp);
}

int DCLayout::execute3()
{
    uint u;
    int i;
    std::map<int, int> rank_clusterrank;
    for (u = 0; u < filter_instances.size(); u++) {
        DCFilterInstance * fi = filter_instances[u];
        rank_clusterrank[fi->dcmpi_rank] = fi->dcmpi_cluster_rank;
    }
    if (has_console_filter) {
        // notify all queues console writes to that it is done
        std::map<std::string, MultiOutportManager*>::iterator it;
        for (it = console_exe->out_ports.begin();
             it != console_exe->out_ports.end();
             it++) {
            MultiOutportManager * mom = it->second;
            for (i = 0; i < mom->num_targets; i++) {
                mom->targets[i]->remove_putter(
                    console_exe->filter_instance->gftid);
            }
        }

        for (i = 0; i < (int)run_hosts_no_console.size(); i++) {
            DCMPIPacket * bye = new DCMPIPacket(
                DCMPIPacketAddress(
                    i, rank_clusterrank[i], -1, "",
                    -1, 0, -1, ""),
                DCMPI_PACKET_TYPE_ALL_MY_FILTERS_DONE, 0);
            console_to_mpi_queue->put(bye);
        }
//         std::set<int>::iterator it3;
//         for (it3 = ranks_console_sends_to.begin();
//              it3 != ranks_console_sends_to.end();
//              it3++) {
//             int other_rank = *it3;
//             DCMPIPacket * bye = new DCMPIPacket(
//                 DCMPIPacketAddress(
//                     other_rank, rank_clusterrank[other_rank], -1, "",
//                     -1, 0, -1, ""),
//                 DCMPI_PACKET_TYPE_ALL_MY_FILTERS_DONE, 0);
//             console_to_mpi_queue->put(bye);
//         }
    }
    console_to_mpi_queue->put(NULL); // tell it to exit, we'll be sending
                                     // nothing more
    console_to_mpi_thread->join();
    console_from_mpi_thread->join();

    int out = 0;
    for (u = 0; u < mpi_command_threads.size(); u++) {
        mpi_command_threads[u]->join();
        if (dcmpi_verbose()) {
            cout << "mpi_run_commands[" << u << "] status is "
                 << mpi_command_threads[u]->getStatus()
                 << endl;
        }
        out = out || mpi_command_threads[u]->getStatus();
    }

    // cleanup anything dynamic for a single execution
    if (uses_unix_socket_console_bridge) {
        remove(consoleToMPITempFn.c_str());
        remove(consoleFromMPITempFn.c_str());
        //rmdir(unix_socket_tempd.c_str());
        dcmpi_rmdir_recursive(unix_socket_tempd);
    }
    uint uT;
    for (uT = 0; uT < mpi_run_temp_files.size(); uT++) {
        remove(mpi_run_temp_files[uT].c_str());
    }
    this->clear_expansion_filter_instances();

    delete init_filter_broadcast;
    init_filter_broadcast = NULL;

    delete console_exe;
    console_exe = NULL;

    console_instance = NULL; // don't delete it; it is owned by user local
                             // console main

    gftids_to_instances.clear();

    ranks_console_sends_to.clear();
    ranks_console_hears_from.clear();

    delete console_to_mpi_queue;
    console_to_mpi_queue = NULL;

    delete console_to_mpi_thread;
    console_to_mpi_thread = NULL;

    delete console_from_mpi_thread;
    console_from_mpi_thread = NULL;

    consoleToMPITempFn = "";
    consoleFromMPITempFn = "";
    mpi_run_temp_files.clear();
    unix_socket_tempd = "";

    delete console_filter;
    console_filter = NULL;

    for (u = 0; u < mpi_command_threads.size(); u++) {
        delete mpi_command_threads[u];
    }
    mpi_command_threads.clear();

    return out;
}

// read hosts from file,1 per line
void DCLayout::set_exec_host_pool_file(const char * hostfile)
{
    char buf[256];
    FILE * f = fopen(hostfile, "r");
    if (!f) {
        std::cerr << "ERROR: in set_exec_host_pool_file(), file "
                  << hostfile << " cannot be opened, aborting"
                  << std::endl << std::flush;
        exit(1);
    }
    exec_host_pool.clear();
    while (fgets(buf, sizeof(buf), f) != NULL) {
        buf[strlen(buf)-1] = 0;
        if (buf[0] != '#') {
            exec_host_pool.push_back(buf);
        }
    }
    fclose(f);
}

void DCLayout::set_exec_host_pool(std::string host)
{
    exec_host_pool.clear();
    exec_host_pool.push_back(host);
}

void DCLayout::set_exec_host_pool(const std::vector<std::string> & hosts)
{
    exec_host_pool = hosts;
}

int DCLayout::execute()
{
    double e1, e2, e3;
    uint u;
    
    // check for console filter, which if present is an error (user should
    // call execute_start() instead)
    for (u = 0; u < filter_instances.size(); u++) {
        DCFilterInstance * fi = filter_instances[u];
        if (fi->filtername == "<console>") {
            std::cerr << "ERROR: if you're using a console filter, you need "
                      << "to call execute_start() and execute_finish(), "
                      << "not execute()"
                      << std::endl << std::flush;
            exit(1);
        }
    }
    

    e1 = dcmpi_doubletime();
    execute1();
    e1 = dcmpi_doubletime() - e1;
    e2 = dcmpi_doubletime();
    execute2();
    e2 = dcmpi_doubletime() - e2;
    e3 = dcmpi_doubletime();
    int out = execute3();
    e3 = dcmpi_doubletime() - e3;
    if (dcmpi_verbose()) {
        std::cerr << "time for execute{1,2,3} is "
                  << e1 << "," << e2 << "," << e3
                  << std::endl << std::flush;
    }
    return out;
}

DCFilter * DCLayout::execute_start()
{
    double e1, e2;

    if (execute_start_called) {
        std::cerr << "ERROR: DCLayout::execute_start() called while another "
                  << "execute_start() was processing!  Please call "
                  << "execute_finish() first.\n";
        exit(1);
    }
    execute_start_called = true;
    e1 = dcmpi_doubletime();
    execute1();
    e1 = dcmpi_doubletime() - e1;
    e2 = dcmpi_doubletime();
    execute2();
    e2 = dcmpi_doubletime() - e2;
    if (dcmpi_verbose()) {
        std::cerr << "time for execute{1,2} is "
                  << e1 << "," << e2
                  << std::endl << std::flush;
    }
    return console_filter;
}

int DCLayout::execute_finish()
{
    int out = 0;
    double e3;
    if (!execute_start_called) {
        std::cerr << "ERROR: DCLayout::execute_finish() called "
                  << "when DCLayout::execute_start() hasn't been called first."
                  << std::endl << std::flush;
        exit(1);
    }
    execute_start_called = false;
    e3 = dcmpi_doubletime();
    out = execute3();
    e3 = dcmpi_doubletime() - e3;
    if (dcmpi_verbose()) {
        std::cerr << "time for execute{3} is "
                  << e3
                  << std::endl << std::flush;    
    }
    return out;
}

std::ostream& operator<<(std::ostream &o, const DCLayout & i)
{
    for (uint u = 0; u < i.filter_instances.size(); u++) {
        DCFilterInstance * fi = i.filter_instances[u];
        o << fi->get_distinguished_name() << endl;
        std::map<std::string, ResolvedMultiPort>::const_iterator it;
        for (it = fi->resolved_outports.begin();
             it != fi->resolved_outports.end();
             it++) {
            const std::string & pn = it->first;
            const ResolvedMultiPort & rmp = it->second;
            o << "    " << pn << " -> {" << rmp << "}\n";
        }
    }
    return o;
}

int DCLayout::get_filter_count_for_dcmpi_rank(int rank)
{
    int c = 0;
    uint u;
    for (u = 0; u < filter_instances.size(); u++) {
        if (filter_instances[u]->dcmpi_rank == rank) {
            c++;
        }
    }
    return c;
}
