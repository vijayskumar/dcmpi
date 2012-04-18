/***************************************************************************
    $Id: DCFilterExecutor.cpp,v 1.26 2008/02/07 05:53:34 krishnan Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

using namespace std;

#include "dcmpi_clib.h"

#include "dcmpi.h"

#include "dcmpi_util.h"
#include "DCFilterExecutor.h"
#include "Gftid_Port.h"
#include "MultiOutportManager.h"
#include "MemUsage.h"
#include <set>

//#define DEBUGK2

DCFilterExecutor::DCFilterExecutor()
{
    this->filter_instance = NULL;
    this->filter_runnable = NULL;
}

DCFilterExecutor::~DCFilterExecutor()
{
    std::map<std::string, Queue<Buffer_MUID> *>::iterator itin;
    for (itin = in_ports.begin(); itin != in_ports.end(); itin++) {
        Queue<Buffer_MUID> * q = itin->second;
        if (q->size()) {
            std::cerr << "WARNING: filter "
                      << this->filter_instance->get_distinguished_name()
                      << " has "
                      << q->size()
                      << " unread buffers remaining "
                      << "on its input port "
                      << itin->first
                      << std::endl << std::flush;
        }
        delete itin->second;
    }

    std::map<std::string, MultiOutportManager*>::iterator itout;
    for (itout = out_ports.begin(); itout != out_ports.end(); itout++) {
        delete itout->second;
    }
}

void DCFilterExecutor::setup(DCFilterInstance * filter_instance,
                             DCFilter * filter_runnable)
{
    this->filter_instance = filter_instance;
    this->filter_runnable = filter_runnable;
    std::map<std::string, std::set<Gftid_Port> >::iterator it;
    for (it = filter_instance->resolved_inports.begin();
         it != filter_instance->resolved_inports.end();
         it++) {
        const std::string & inport = it->first;
        std::set<Gftid_Port> & set_of_gftid_ports = it->second;
        std::set<int> putters;
        std::set<Gftid_Port>::iterator it2;
        for (it2 = set_of_gftid_ports.begin();
             it2 != set_of_gftid_ports.end();
             it2++) {
            putters.insert(it2->gftid);
        }
        Queue<Buffer_MUID> * q = new Queue<Buffer_MUID>();
        q->set_putters(putters);
        this->in_ports[inport] = q;
    }
}

Buffer_MUID DCFilterExecutor::read(const std::string & port)
{
    if (this->in_ports.count(port) == 0)
    {
        std::cerr << "ERROR: bad read port " << port
                  << " in filter "
                  << this->filter_instance->get_distinguished_name()
                  << endl
                  << "These are its valid read ports:\n";
        std::vector<std::string> in_ports = get_in_port_names();
        for (uint u = 0; u < in_ports.size(); u++) {
            cout << "    " << in_ports[u] << endl;
        }
        exit(1);
    }
    if (dcmpi_verbose()) {
      std::cout << filter_instance->get_instance_name() << " issued read on port " << port << std::endl;
    }
    Buffer_MUID ret = this->in_ports[port]->get();
    if (dcmpi_verbose()) {
      std::cout << filter_instance->get_instance_name() << " finished read on port " << port << std::endl;
    }

    return ret;
}

Buffer_MUID DCFilterExecutor::read_nonblocking(const std::string & port)
{
    if (this->in_ports.count(port) == 0)
    {
        std::cerr << "ERROR: bad read port " << port
                  << " in filter "
                  << this->filter_instance->get_distinguished_name()
                  << endl
                  << "These are its valid read ports:\n";
        std::vector<std::string> in_ports = get_in_port_names();
        for (uint u = 0; u < in_ports.size(); u++) {
            cout << "    " << in_ports[u] << endl;
        }
        exit(1);
    }
    Buffer_MUID out;
    Queue<Buffer_MUID> * q = in_ports[port];
    q->get_if_item_present(out);
    return out;
}

bool DCFilterExecutor::any_upstream_running()
{
    std::map<std::string, Queue<Buffer_MUID> *>::iterator it;
    for (it = in_ports.begin();
         it != in_ports.end();
         it++) {
        Queue<Buffer_MUID> * q = it->second;
        if (q->has_more_putters()) {
            return true;
        }
    }
    return false;
}

Buffer_MUID DCFilterExecutor::read_until_upstream_exit(const std::string & port)
{
    if (this->in_ports.count(port) == 0)
    {
        std::cerr << "ERROR: bad read port " << port
                  << " in filter "
                  << this->filter_instance->get_distinguished_name()
                  << endl
                  << "These are its valid read ports:\n";
        std::vector<std::string> in_ports = get_in_port_names();
        for (uint u = 0; u < in_ports.size(); u++) {
            cout << "    " << in_ports[u] << endl;
        }
        exit(1);
    }
    double sleep_time = 0.001;
    Buffer_MUID out;
    Queue<Buffer_MUID> * q = in_ports[port];
    while (1)
    {
        bool has_more_putters = false;
        if (q->get_if_item_present(out)) {
            return out;
        }
        else if (q->has_more_putters()) {
            has_more_putters = true;
        }
        if (has_more_putters) {
            dcmpi_doublesleep(sleep_time);
            if (sleep_time < 0.01)
            {
                sleep_time *= 2;
            }
        }
        else {
            break;
        }
    }
    return out;
}

Buffer_MUID DCFilterExecutor::readany(std::string * port)
{
    double sleep_time = 0.001;
    Buffer_MUID out;

    /** start with random location and iterate over rest */
    int maxv = in_ports.size();
    int randomlocation = (int) ((float) maxv * (double)dcmpi_rand()/(RAND_MAX+1.0));
	
    /** random location is a number in [0, in_ports.size()] */
    std::map<std::string, Queue<Buffer_MUID> *>::iterator startpos;
    int counter = 0;
    for (startpos=in_ports.begin(); counter < randomlocation;) {
        counter++; startpos++;
    }
    
    while (1)
    {
        bool has_more_putters = false;
        std::map<std::string, Queue<Buffer_MUID> *>::iterator it;        

#ifdef DEBUGK2
	//changes by k2
	std::set<std::string> portsNotDone;
#endif
	
        for (it = startpos, counter = 0; counter < maxv; counter++) {
            if (it->second->get_if_item_present(out)) {
                if (port) {
                    *port = it->first;
                }
                return out;
            }
            else if (it->second->has_more_putters()) {
                has_more_putters = true;

#ifdef DEBUGK2		
		portsNotDone.insert(it->first);
#endif
            }
            it++;
            if (it==in_ports.end()) {
                it = in_ports.begin();
            }
        }

        if (has_more_putters) {
            dcmpi_doublesleep(sleep_time);
            if (sleep_time < 0.01)
            {
                sleep_time *= 2;
            }

#ifdef DEBUGK2
   	    /** who are the putters left : k2 */
	    //	    if (sleep_time > 2.0) {
	      if (dcmpi_verbose()) {
		std::cout << "Ports still left are: " ;
		
		for (std::set<std::string>::iterator it = portsNotDone.begin(); it!=portsNotDone.end(); it++)
		  std::cout << *it << " ";
		std::cout << std::endl;
	      }
	      //}
#endif
        }
        else {
            break;
        }
    }
    return out;
}

void DCFilterExecutor::write(DCBuffer * buf,
                             const std::string & port,
                             const std::string & label,
                             int port_ticket)
{
    if (this->out_ports.count(port) == 0) {
        std::cerr << "ERROR: invalid write port " << port
                  << " in filter "
                  << this->filter_instance->get_distinguished_name()
                  << endl
                  << "These are its valid write ports:\n";
        std::vector<std::string> out_ports = get_out_port_names();
        for (uint u = 0; u < out_ports.size(); u++) {
            cerr << "    " << out_ports[u] << endl;
        }
        exit(1);
    }
    this->out_ports[port]->select_target(label, port_ticket)->feed(buf);
}

void DCFilterExecutor::writeany(DCBuffer * buf)
{
    uint num_keys = this->out_ports.size();
    if (num_keys == 0) {
        std::cerr << "ERROR: no ports to writeany() to in filter "
                  << filter_instance->filtername << " (instance name "
                  << (filter_instance->instancename) << ")\n"
                  << "    make sure that a filter that uses writeany "
                  << "has at least 1 outgoing port."
                  << std::endl << std::flush;
        exit(1);
    }
    else if (num_keys == 1) {
        return this->write(buf, out_ports.begin()->first, "", -1);
    }
    else {
        // choose a random port, does this make sense?
        int idx = dcmpi_rand() % num_keys;
        std::map<std::string, MultiOutportManager*>::iterator it;
        it = this->out_ports.begin();
        for (int i = 0; i < idx; i++) {
            it++;
        }
        std::string port = it->first;
        return this->write(buf, port, "", -1);
    }
}

void DCFilterExecutor::write_broadcast(
    DCFilter * caller, DCBuffer * buf, const std::string & port)
{
    if (this->out_ports.count(port) == 0) {
        std::cerr << "ERROR: invalid write_broadcast port " << port
                  << " in filter "
                  << this->filter_instance->get_distinguished_name()
                  << endl
                  << "These are its valid write ports:\n";
        std::vector<std::string> out_ports = get_out_port_names();
        for (uint u = 0; u < out_ports.size(); u++) {
            cout << "    " << out_ports[u] << endl;
        }
        exit(1);
    }
    if (caller->stats.write_block_time.count(port) == 0) {
        caller->stats.write_block_time[port] = 0.0;
    }
    for (int i = 0; i < out_ports[port]->get_num_targets(); i++) {
        double before = dcmpi_doubletime();
        if (!dcmpi_is_local_console) {
            MUID_MemUsage[filter_instance->gftid]->accumulate(buf->getUsedSize());
        }
        else {
            local_console_outgoing_memusage.accumulate(buf->getUsedSize());
        }
        caller->stats.write_block_time[port] += dcmpi_doubletime() - before;    
        DCBuffer * b2 = new DCBuffer(*buf);
        out_ports[port]->select_target("", i)->feed(b2);
    }
}

int DCFilterExecutor::get_last_write_ticket(const std::string & port)
{
    if (this->out_ports.count(port) == 0) {
        std::cerr << "ERROR: in DCFilterExecutor::get_last_write_ticket(): "
                  << "bad port " << port
                  << " in filter " << this->filter_instance->filtername
                  << " (" << this->filter_instance->instancename << ") "
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    int outv = this->out_ports[port]->get_last_chosen_target();
    if (outv == -1) {
        std::cerr << "ERROR: in DCFilterExecutor::get_last_write_ticket(): "
                  << "this method was called before any write occured to port "
                  << port
                  << " in filter "
                  << this->filter_instance->get_distinguished_name()
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    return outv;
}

std::vector<std::string> DCFilterExecutor::get_in_port_names()
{
    std::vector<std::string> out;
    std::map<std::string, Queue<Buffer_MUID> *>::iterator it;
    for (it = this->in_ports.begin(); it != this->in_ports.end(); it++) {
        out.push_back(it->first);
    }
    return out;
}

std::vector<std::string> DCFilterExecutor::get_out_port_names()
{
    std::vector<std::string> out;
    std::map<std::string, MultiOutportManager*>::iterator it;
    for (it = this->out_ports.begin(); it != this->out_ports.end(); it++) {
        out.push_back(it->first);
    }
    return out;
}
