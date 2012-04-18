#ifndef MULTIOUTPORTMANAGER_H
#define MULTIOUTPORTMANAGER_H

/***************************************************************************
    $Id: MultiOutportManager.h,v 1.8 2005/04/18 15:09:20 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include "dcmpi_clib.h"

#include "QueueWriter.h"

typedef enum DCMPI_MULTIOUT_POLICY
{
    DCMPI_MULTIOUT_POLICY_ROUND_ROBIN = 0,
    DCMPI_MULTIOUT_POLICY_RANDOM
} DCMPI_MULTIOUT_POLICY;

class MultiOutportManager
{
    int policy;
    int rr_nextvictim;
    int last_chosen_target;
    DCFilterInstance* local_instance;
public:
    int num_targets;
    std::vector<QueueWriter*> targets;
    std::map<std::string, std::set<int> > label_to_targets;
    std::map<std::string, std::set<int>::iterator> label_to_targets_pos;
    ~MultiOutportManager()
    {
        for (uint u = 0; u < targets.size(); u++) {
            delete targets[u];
        }
    }
    MultiOutportManager(int policy, DCFilterInstance* local_instance)
    {
        this->policy = policy;
        this->local_instance = local_instance;
        rr_nextvictim = 0;
        num_targets = 0;
        last_chosen_target = -1;
    }
    void add_target(QueueWriter * target) {
        targets.push_back(target);
        num_targets++;
    }
    int get_num_targets() { return num_targets; }
    void note_final_target();
    int get_last_chosen_target() {
        return last_chosen_target;
    }
    QueueWriter * select_target(std::string label, int port_ticket);
    std::map< std::string, std::vector<DCFilterInstance*> > get_labels()
    {
        std::map<std::string, std::vector<DCFilterInstance*> > out_value;
        std::map<std::string, std::set<int> >::iterator it;
        for (it = label_to_targets.begin(); it != label_to_targets.end();
             it++) {
            std::set<int>::iterator it2;
            for (it2 = it->second.begin();
                 it2 != it->second.end();
                 it2++) {
                if (out_value[it->first].size()==0){
                    out_value[it->first] = std::vector<DCFilterInstance*>();
                }
                out_value[it->first].push_back(
                    targets[*it2]->get_remote_instance());
            }
        }
        return out_value;
    }
private:
    MultiOutportManager(){}
};

#endif /* #ifndef MULTIOUTPORTMANAGER_H */
