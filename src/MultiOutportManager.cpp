#include "dcmpi_clib.h"

#include "QueueWriter.h"

#include "MultiOutportManager.h"

#include "dcmpi_backtrace.h"

using namespace std;

void MultiOutportManager::note_final_target()
{
    // randomize order of output targets to try to reduce endpoint
    // contention
    std::random_shuffle(targets.begin(), targets.end());
        
    int i;
    uint u;
    for (i = 0; i < num_targets; i++) {
        QueueWriter * target = targets[i];
        DCFilterInstance* fi = target->get_remote_instance();
        std::vector<std::string> & runtime_labels = fi->runtime_labels;
        for (u = 0; u < runtime_labels.size(); u++) {
            std::string & l = runtime_labels[u];
            if (label_to_targets.count(l)== 0) {
                label_to_targets[l] = std::set<int>();
            }
            label_to_targets[l].insert(i);
        }
    }
    std::map<std::string, std::set<int> >::iterator it;
    for (it = label_to_targets.begin();
         it != label_to_targets.end();
         it++) {
        label_to_targets_pos[it->first] = it->second.begin();
    }
}

QueueWriter * MultiOutportManager::select_target(
    std::string label, int port_ticket)
{
    if (!label.empty()) {
        if (label_to_targets.count(label) == 0) {
            std::cerr << "ERROR: in filter "
                      << local_instance->get_distinguished_name()
                      << ": attempt was made to write using a label "
                      << "of '" << label << "', which is not present "
                      << "for any remote targets\n";
            dcmpi_print_backtrace();
            assert(0);
        }
        last_chosen_target = *(label_to_targets_pos[label]);
        label_to_targets_pos[label]++;
        if (label_to_targets_pos[label] == label_to_targets[label].end()) {
            label_to_targets_pos[label] = label_to_targets[label].begin();
        }
    }
    else if (port_ticket != -1) {
        if ((port_ticket < 0) || (port_ticket >= (int)targets.size())) {
            std::cerr << "ERROR: invalid port ticket " << port_ticket
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
        last_chosen_target = port_ticket;
    }
    else if (num_targets == 1) {
        last_chosen_target = 0;
    }
    else if (policy == DCMPI_MULTIOUT_POLICY_ROUND_ROBIN) {
        last_chosen_target = rr_nextvictim;
        rr_nextvictim++;
        if (rr_nextvictim == num_targets)
            rr_nextvictim = 0;
    }
    else if (policy == DCMPI_MULTIOUT_POLICY_RANDOM) {
        last_chosen_target = dcmpi_rand() % num_targets;
    }
    else {
        assert(0);
    }
    return targets[last_chosen_target];
}
