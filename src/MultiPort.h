#ifndef MULTIPORT_H
#define MULTIPORT_H

/***************************************************************************
    $Id: MultiPort.h,v 1.4 2006/11/15 20:13:49 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include "Instance_Port.h"

class Instance_Port;

class MultiPort
{
public:
    std::vector<Instance_Port> remotes;
    MultiPort(DCFilterInstance * remote_filter_instance = NULL,
              std::string remote_port_name = "")
    {
        if (remote_filter_instance && remote_port_name != "") {
            this->add_remote(remote_filter_instance, remote_port_name);
        }
    }
    void add_remote(DCFilterInstance *remote_filter_instance,
                    std::string remote_port_name)
    {
        Instance_Port ip(remote_filter_instance, remote_port_name);
//         if (std::find(remotes.begin(),
//                       remotes.end(),
//                       ip)==remotes.end()) {
        this->remotes.push_back(ip);
//         }
//         else {
//             std::cerr << "WARNING: remote instance "
//                       << remote_filter_instance
//                       << " and port " << remote_port_name
//                       << " already exists at " << __FILE__ << ":" << __LINE__
//                       << std::endl << std::flush;
//         }
    }
};

#endif /* #ifndef MULTIPORT_H */
