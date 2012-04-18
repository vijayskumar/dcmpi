/***************************************************************************
    $Id: DCFilter.cpp,v 1.27 2006/05/26 22:06:53 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include "dcmpi_clib.h"

#include "dcmpi.h"
#include "DCFilterExecutor.h"
#include "MultiOutportManager.h"
#include "MemUsage.h"

#include "dcmpi_util.h"

using namespace std;

DCFilter::DCFilter()
{
    this->executor = NULL;
    this->init_filter_broadcast = NULL;
}

DCBuffer * DCFilter::get_init_filter_broadcast()
{
    return init_filter_broadcast;
}

std::string DCFilter::get_filter_name()
{
    return this->executor->filter_instance->filtername;
}

std::string DCFilter::get_instance_name()
{
    return this->executor->filter_instance->instancename;
}

std::string DCFilter::get_distinguished_name()
{
    return this->executor->filter_instance->get_distinguished_name();
}

int DCFilter::get_global_filter_thread_id()
{
    return this->executor->filter_instance->gftid;
}

std::string DCFilter::get_bind_host()
{
    return this->executor->filter_instance->given_bind_host;
}

std::vector<std::string> DCFilter::get_in_port_names()
{
    return this->executor->get_in_port_names();
}

std::vector<std::string> DCFilter::get_out_port_names()
{
    return this->executor->get_out_port_names();
}

bool DCFilter::has_in_port(const std::string & port)
{
    return this->executor->in_ports.count(port)>0;
}
bool DCFilter::has_out_port(const std::string & port)
{
    return this->executor->out_ports.count(port)>0;
}

int DCFilter::get_copy_rank()
{
    return this->executor->filter_instance->copy_rank;
}

int DCFilter::get_num_copies()
{
    return this->executor->filter_instance->num_copies;
}

int DCFilter::process() {
    return -1;
}

DCLayout * DCFilter::get_composite_layout(std::vector<std::string> cluster_hosts) {
    return NULL;
}

DCBuffer * DCFilter::read(const std::string & port)
{
    double before = dcmpi_doubletime();
    Buffer_MUID bm = this->executor->read(port);
    if (stats.read_block_time.count(port) == 0) {
        stats.read_block_time[port] = 0.0;
    }
    stats.read_block_time[port] += dcmpi_doubletime() - before;
    DCBuffer * out = bm.buffer;
    if (!dcmpi_is_local_console) {
        MUID_MemUsage[bm.muid]->reduce(out->getUsedSize());
    }
    return out;
}

DCBuffer * DCFilter::read_nonblocking(const std::string & port)
{
    Buffer_MUID bm = this->executor->read_nonblocking(port);
    if (stats.read_block_time.count(port) == 0) {
        stats.read_block_time[port] = 0.0;
        // but don't accumulate any time, since we're non-blocking
    }
    DCBuffer * out = bm.buffer;
    if (out && !dcmpi_is_local_console) {
        MUID_MemUsage[bm.muid]->reduce(out->getUsedSize());
    }
    return out;
}

DCBuffer * DCFilter::read_until_upstream_exit(const std::string & port)
{
    double before = dcmpi_doubletime();
    Buffer_MUID bm = this->executor->read_until_upstream_exit(port);
    if (stats.read_block_time.count(port) == 0) {
        stats.read_block_time[port] = 0.0;
    }
    stats.read_block_time[port] += dcmpi_doubletime() - before;
    DCBuffer * out = bm.buffer;
    if (out && !dcmpi_is_local_console) {
        MUID_MemUsage[bm.muid]->reduce(out->getUsedSize());
    }
    return out;
}

bool DCFilter::any_upstream_running()
{
    return this->executor->any_upstream_running();
}

DCBuffer * DCFilter::readany(std::string * port)
{
    double before = dcmpi_doubletime();
    Buffer_MUID bm = this->executor->readany(port);
    stats.read_block_time[port?(*port):"any"] += dcmpi_doubletime() - before;
    DCBuffer * out = bm.buffer;
    if (out && !dcmpi_is_local_console) {
        MUID_MemUsage[bm.muid]->reduce(out->getUsedSize());
    }
    return out;
}

void DCFilter::write(DCBuffer * buf, const std::string & port)
{
    double before = dcmpi_doubletime();
    if (!dcmpi_is_local_console) {
        MUID_MemUsage[this->get_global_filter_thread_id()]->accumulate(buf->getUsedSize());
    }
    else {
        local_console_outgoing_memusage.accumulate(buf->getUsedSize());
    }
    if (stats.write_block_time.count(port) == 0) {
        stats.write_block_time[port] = 0.0;
    }
    stats.write_block_time[port] += dcmpi_doubletime() - before;    
    DCBuffer * b2 = new DCBuffer(*buf);
    return this->executor->write(b2, port, "", -1);
}

void DCFilter::write(DCBuffer * buf,
                     const std::string & port,
                     const std::string & label)
{
    double before = dcmpi_doubletime();
    if (!dcmpi_is_local_console) {
        MUID_MemUsage[this->get_global_filter_thread_id()]->accumulate(buf->getUsedSize());    
    }
    else {
        local_console_outgoing_memusage.accumulate(buf->getUsedSize());
    }
    if (stats.write_block_time.count(port) == 0) {
        stats.write_block_time[port] = 0.0;
    }
    stats.write_block_time[port] += dcmpi_doubletime() - before;    
    DCBuffer * b2 = new DCBuffer(*buf);
    return this->executor->write(b2, port, label, -1);
}

void DCFilter::write(DCBuffer * buf, const std::string & port, int port_ticket)
{
    double before = dcmpi_doubletime();
    if (!dcmpi_is_local_console) {
        MUID_MemUsage[this->get_global_filter_thread_id()]->accumulate(buf->getUsedSize());
    }
    else {
        local_console_outgoing_memusage.accumulate(buf->getUsedSize());
    }
    if (stats.write_block_time.count(port) == 0) {
        stats.write_block_time[port] = 0.0;
    }
    stats.write_block_time[port] += dcmpi_doubletime() - before;    
    DCBuffer * b2 = new DCBuffer(*buf);
    return this->executor->write(b2, port, "", port_ticket);
}

void DCFilter::write_broadcast(DCBuffer * buf, const std::string & port)
{
    return this->executor->write_broadcast(this, buf, port);
}

void DCFilter::write_nocopy(DCBuffer * buf, const std::string & port)
{
    double before = dcmpi_doubletime();
    if (!dcmpi_is_local_console) {
        MUID_MemUsage[this->get_global_filter_thread_id()]->accumulate(buf->getUsedSize());
    }
    else {
        local_console_outgoing_memusage.accumulate(buf->getUsedSize());
    }
    if (stats.write_block_time.count(port) == 0) {
        stats.write_block_time[port] = 0.0;
    }
    stats.write_block_time[port] += dcmpi_doubletime() - before;    
    return this->executor->write(buf, port, "", -1);
}

void DCFilter::write_nocopy(DCBuffer * buf,
                            const std::string & port,
                            const std::string & label)
{
    double before = dcmpi_doubletime();
    if (!dcmpi_is_local_console) {
        MUID_MemUsage[this->get_global_filter_thread_id()]->accumulate(buf->getUsedSize());
    }
    else {
        local_console_outgoing_memusage.accumulate(buf->getUsedSize());
    }
    if (stats.write_block_time.count(port) == 0) {
        stats.write_block_time[port] = 0.0;
    }
    stats.write_block_time[port] += dcmpi_doubletime() - before;    
    return this->executor->write(buf, port, label, -1);
}

void DCFilter::write_nocopy(DCBuffer * buf,
                            const std::string & port,
                            int port_ticket)
{
    double before = dcmpi_doubletime();
    if (!dcmpi_is_local_console) {
        MUID_MemUsage[this->get_global_filter_thread_id()]->accumulate(buf->getUsedSize());
    }
    else {
        local_console_outgoing_memusage.accumulate(buf->getUsedSize());
    }
    if (stats.write_block_time.count(port) == 0) {
        stats.write_block_time[port] = 0.0;
    }
    stats.write_block_time[port] += dcmpi_doubletime() - before;    
    return this->executor->write(buf, port, "", port_ticket);
}


void DCFilter::writeany(DCBuffer * buf)
{
    double before = dcmpi_doubletime();
    if (!dcmpi_is_local_console) {
        MUID_MemUsage[this->get_global_filter_thread_id()]->accumulate(buf->getUsedSize());
    }
    else {
        local_console_outgoing_memusage.accumulate(buf->getUsedSize());
    }
    if (stats.write_block_time.count("any") == 0) {
        stats.write_block_time["any"] = 0.0;
    }
    stats.write_block_time["any"] += dcmpi_doubletime() - before;    
    DCBuffer * b2 = new DCBuffer(*buf);
    return this->executor->writeany(b2);
}

void DCFilter::writeany_nocopy(DCBuffer * buf)
{
    double before = dcmpi_doubletime();
    if (!dcmpi_is_local_console) {
        MUID_MemUsage[this->get_global_filter_thread_id()]->accumulate(buf->getUsedSize());
    }
    else {
        local_console_outgoing_memusage.accumulate(buf->getUsedSize());
    }
    if (stats.write_block_time.count("any") == 0) {
        stats.write_block_time["any"] = 0.0;
    }
    stats.write_block_time["any"] += dcmpi_doubletime() - before;    
    return this->executor->writeany(buf);
}

int DCFilter::get_last_write_ticket(const std::string & port)
{
    return this->executor->get_last_write_ticket(port);
}

bool DCFilter::has_param(const std::string & key)
{
    return this->executor->filter_instance->user_params.count(key) != 0;
}

std::string DCFilter::get_param(const std::string & key)
{
    if (this->executor->filter_instance->user_params.count(key) == 0) {
        std::cerr << "ERROR: invalid get_param call on key '" << key
                  << "' in filter " << get_distinguished_name()
                  << std::endl << std::flush;
        exit(1);
    }
    return this->executor->filter_instance->user_params[key];
}

int DCFilter::get_param_as_int(const std::string & key)
{
    std::string s = get_param(key);
    return atoi(s.c_str());
}

double DCFilter::get_param_as_double(const std::string & key)
{
    std::string s = get_param(key);
    return atof(s.c_str());
}

DCBuffer DCFilter::get_param_buffer(std::string key)
{
    if (this->executor->filter_instance->user_buffer_params.count(key) == 0) {
        std::cerr << "ERROR: invalid get_param_buffer call on key '" << key
                  << "' in filter " << get_distinguished_name()
                  << std::endl << std::flush;
        exit(1);
    }
    return this->executor->filter_instance->user_buffer_params[key];
}

std::map<std::string, vector<DCFilterInstance*> > DCFilter::get_write_labels(
    std::string port)
{
    if (!this->executor->out_ports.count(port)) {
        std::cerr << "ERROR: no such port " << port
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    return this->executor->out_ports[port]->get_labels();
}
