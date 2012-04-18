#ifndef DCFILTEREXECUTOR_H
#define DCFILTEREXECUTOR_H

/***************************************************************************
    $Id: DCFilterExecutor.h,v 1.10 2005/04/18 19:49:50 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include "dcmpi_clib.h"

#include "Queue.h"

class DCFilter;
class DCFilterInstance;
class DCBuffer;
class Buffer_MUID;
class MultiOutportManager;

class DCFilterExecutor
{
public:
    DCFilterInstance * filter_instance;
    DCFilter * filter_runnable;

    ~DCFilterExecutor();

    std::map<std::string, Queue<Buffer_MUID> *> in_ports;
    std::map<std::string, MultiOutportManager*> out_ports;

    DCFilterExecutor();
    void setup(DCFilterInstance * filter_instance,
               DCFilter * filter_runnable);
    Buffer_MUID read(const std::string & port);
    Buffer_MUID read_nonblocking(const std::string & port);
    bool any_upstream_running();
    Buffer_MUID read_until_upstream_exit(const std::string & port);
    Buffer_MUID readany(std::string * port);
    void write(DCBuffer * buf,
               const std::string & port,
               const std::string & label,
               int port_ticket);
    void writeany(DCBuffer * buf);
    void write_broadcast(DCFilter * caller,
                         DCBuffer * buf, const std::string & port);
    int get_last_write_ticket(const std::string & port);
    std::vector<std::string> get_in_port_names();
    std::vector<std::string> get_out_port_names();    
};

#endif /* #ifndef DCFILTEREXECUTOR_H */
