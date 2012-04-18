/***************************************************************************
    $Id: DCFilterInstance.cpp,v 1.24 2006/05/09 09:16:29 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include "dcmpi_clib.h"

#include "dcmpi.h"

#include "dcmpi_util.h"
#include "MultiPort.h"
#include "ResolvedMultiPort.h"

using namespace std;

DCFilterInstance::DCFilterInstance(
    std::string filtername,
    std::string instancename) :
    heritage(""),
    filtername(filtername),
    instancename(instancename),
    copy_rank(0),
    num_copies(1),
    local_copy_rank(0),
    local_num_copies(1),
    transparent_copies_expanded(false),
    is_inbound_composite_designate(false),
    is_outbound_composite_designate(false),
    gftid(-1),
    dcmpi_rank(-1),
    dcmpi_cluster_rank(0)
{
    ;
}

DCFilterInstance::DCFilterInstance() :
    heritage(""),
    filtername(""),
    instancename(""),
    copy_rank(0),
    num_copies(1),
    local_copy_rank(0),
    local_num_copies(1),
    transparent_copies_expanded(false),
    is_inbound_composite_designate(false),
    is_outbound_composite_designate(false),
    gftid(-1),
    dcmpi_rank(-1)
{
    ;
}

DCFilterInstance::~DCFilterInstance()
{
    ;
}

void DCFilterInstance::serialize(DCBuffer * buf) const
{
    uint u;
    
    buf->Append(heritage);
    buf->Append(filtername);
    buf->Append(instancename);
    buf->Append((int4)copy_rank);
    buf->Append((int4)num_copies);
    buf->Append((int4)local_copy_rank);
    buf->Append((int4)local_num_copies);
    buf->Append((int4)gftid);

    buf->Append((int4)resolved_outports.size());
    std::map<std::string, ResolvedMultiPort>::const_iterator it;
    for (it = resolved_outports.begin(); it != resolved_outports.end(); it++) {
        buf->Append(it->first);
        it->second.serialize(buf);
    }

    buf->Append((int4)resolved_inports.size());
    std::map<std::string, std::set<Gftid_Port> >::const_iterator it2;
    for (it2 = resolved_inports.begin(); it2 != resolved_inports.end(); it2++) {
        buf->Append(it2->first);
        const std::set<Gftid_Port> & s = it2->second;
        buf->Append((int4)s.size());
        std::set<Gftid_Port>::const_iterator it3;
        for (it3 = s.begin(); it3 != s.end(); it3++) {
            it3->serialize(buf);
        }
    }

    buf->Append((int4)user_params.size());
    std::map<std::string, std::string>::const_iterator it4;
    for (it4 = user_params.begin(); it4 != user_params.end();it4++) {
        buf->Append(it4->first);
        buf->Append(it4->second);
    }
    buf->Append((int4)user_buffer_params.size());
    std::map<std::string, DCBuffer>::const_iterator it5;
    for (it5 = user_buffer_params.begin();
         it5 != user_buffer_params.end();
         it5++) {
        buf->Append(it5->first);
        DCBuffer b(it5->second);
        b.saveToDCBuffer(buf);
    }

    buf->Append((int4)dcmpi_rank);
    buf->Append((int4)dcmpi_cluster_rank);
    buf->Append(dcmpi_host);
    buf->Append(given_bind_host);
    buf->Append((int4)runtime_labels.size());
    for (u = 0; u < runtime_labels.size(); u++) {
        buf->Append(runtime_labels[u]);
    }
}

void DCFilterInstance::deSerialize(DCBuffer * buf)
{
    int4 i4;
    int4 sz, sz2;
    int4 i, i2;
    std::string s;
    ResolvedMultiPort rmp;
    Gftid_Port gp;
    std::set<Gftid_Port> gftid_port_set;
    buf->Extract(&heritage);
    buf->Extract(&filtername);
    buf->Extract(&instancename);
    buf->Extract(&i4); copy_rank = i4;
    buf->Extract(&i4); num_copies = i4;
    buf->Extract(&i4); local_copy_rank = i4;
    buf->Extract(&i4); local_num_copies = i4;
    buf->Extract(&i4); gftid = i4;

    buf->Extract(&sz); // size of resolved_outports
    for (i = 0; i < sz; i++) {
        buf->Extract(&s);
        rmp.deSerialize(buf);
        resolved_outports[s] = rmp;
    }

    buf->Extract(&sz); // size of resolved_inports
    for (i = 0; i < sz; i++) {
        buf->Extract(&s);
        gftid_port_set.clear();
        buf->Extract(&sz2); // size of set
        for (i2 = 0; i2 < sz2; i2++) {
            gp.deSerialize(buf);
            gftid_port_set.insert(gp);
        }
        resolved_inports[s] = gftid_port_set;
    }

    buf->Extract(&sz); // size of user_params
    for (i = 0; i < sz; i++) {
        buf->Extract(&s);
        buf->Extract(&user_params[s]);
    }

    buf->Extract(&sz); // size of user_buffer_params
    for (i = 0; i < sz; i++) {
        buf->Extract(&s);
        DCBuffer b;
        b.restoreFromDCBuffer(buf);
        user_buffer_params[s] = b;
    }

    buf->Extract(&i4); dcmpi_rank = i4;
    buf->Extract(&i4); dcmpi_cluster_rank = i4;
    buf->Extract(&dcmpi_host);
    buf->Extract(&given_bind_host);
    buf->Extract(&i4);
    for (i = 0; i < i4; i++) {
        std::string s;
        buf->Extract(&s);
        runtime_labels.push_back(s);
    }
}

std::string DCFilterInstance::get_filter_name()
{
    return this->filtername;
}

std::string DCFilterInstance::get_instance_name()
{
    return this->instancename;
}

std::string DCFilterInstance::get_distinguished_name()
{
    std::string s = this->heritage;
    if (s != "") {
        s += "___";
    }
    s += "F";
    s += this->filtername;
    s += "_I";
    s += this->instancename;
    return s;
}

void DCFilterInstance::to_graphviz(
    DCLayout *                 layout_obj,
    bool                       print_host_if_applicable,
    std::string &              instance_text,
    std::vector<std::string> & connections)
{
    DCFilterInstance * loci = this;
    std::string locstr = loci->get_distinguished_name();
    if (loci->num_copies > 1) {
        locstr += "(";
        locstr += dcmpi_to_string(loci->copy_rank);
        locstr += "/";
        locstr += dcmpi_to_string(loci->num_copies);
        locstr += ")";
        locstr += "(L";
        locstr += dcmpi_to_string(loci->local_copy_rank);
        locstr += "/";
        locstr += dcmpi_to_string(loci->local_num_copies);
        locstr += ")";
    }
    std::string s = "";
    if (print_host_if_applicable) {
        if (!loci->dcmpi_host.empty()) {
            locstr += "_H";
            locstr += loci->dcmpi_host;
        }
    }
    if (dcmpi_verbose()) {
        if (loci->gftid != -1) {
            locstr += "_ID";
            locstr += dcmpi_to_string(loci->gftid);
        }
    }
    std::map<std::string, MultiPort>::iterator it;
    for (it = outports.begin(); it != outports.end(); it++) {
        const std::string & fromport = it->first;
        MultiPort & mp = (it->second);
        std::vector<Instance_Port>::iterator it2;
        for (it2 = mp.remotes.begin(); it2 != mp.remotes.end(); it2++) {
            const std::string & toport = it2->port;
            DCFilterInstance * remi = it2->instance;
            std::string conn;
            conn += dcmpi_to_string(loci->gftid);
            conn += " -> ";
            conn += dcmpi_to_string(remi->gftid);
            conn += " [color=red,arrowsize=0.75,headport=s,tailport=n,";
            conn += "fontcolor=brown,fontname=Courier,fontsize=12,";
            conn += "headlabel=\"" + toport;
            conn += "\",taillabel=\"" + fromport + "\"]";
            connections.push_back(conn);
        }
    }
    s += dcmpi_to_string(loci->gftid);
    s += " [label=\"";
    s += locstr;
    s += "\"]\n";
    instance_text = s;
}

void DCFilterInstance::set_param(std::string key, int val)
{
    this->set_param(key, dcmpi_to_string(val));
}

static void hosts_bind_internal(
    DCFilterInstance * filter,
    const std::vector<std::string> & hostnames)
{
    if (filter->bind_hosts.size()) {
        filter->bind_hosts.clear();
    }
    uint u;
    filter->num_copies = hostnames.size();
    for (u = 0; u < hostnames.size(); u++) {
        const std::string & host = hostnames[u];
        filter->bind_hosts.push_back(host);
    }
    filter->add_labels_to_transparents(hostnames);
}

static void host_bind_internal(
    DCFilterInstance * filter,
    const std::string & hostname)
{
    if (filter->bind_hosts.size()) {
        filter->bind_hosts.clear();
    }
    filter->bind_hosts.push_back(hostname);
    filter->num_copies = 1;
    filter->add_label(hostname);
}

void DCFilterInstance::bind_to_host(const std::string & host)
{
    host_bind_internal(this, host);
}

void DCFilterInstance::add_label(const std::string & label)
{
    if (this->num_copies > 2) {
        std::cerr << "ERROR: don't call add_label on a transparent copy filter,"
                  << " call add_labels_to_transparents instead\n";
        exit(1);
    }

    if (layout_labels.empty()) {
        layout_labels.push_back(std::vector<std::string>());
    }
    layout_labels[0].push_back(label);
}

void DCFilterInstance::make_transparent(int num_copies)
{
    if (this->filtername == "<console>") {
        std::cerr << "ERROR: cannot make a console filter transparent."
                  << std::endl << std::flush;
        exit(1);
    }
    else if (num_copies <= 0) {
        std::cerr << "ERROR:  DCFilterInstance::make_transparent() "
                  << "called with num_copies argument <= 0"
                  << std::endl << std::flush;
        exit(1);
    }
    int i;
    for (i = 0; i < num_copies; i++) {
        this->bind_hosts.push_back("");
    }
    this->num_copies = bind_hosts.size();
}

void DCFilterInstance::bind_to_hosts(
    const std::vector<std::string> & hosts)
{
    hosts_bind_internal(this, hosts);
}

void DCFilterInstance::add_labels_to_transparents(
    const std::vector<std::string> & labels)
{
    if (num_copies < 2) {
        std::cerr << "ERROR:  you are calling the method "
                  << "add_labels_to_transparents() "
                  << "on a non-transparent copy instance.  "
                  << "You should call the method add_label() instead."
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    if ((int)(labels.size())!=num_copies) {
        std::cerr << "ERROR:  invalid number of labels given to the method "
                  << "add_labels_to_transparents() "
                  << "(give as many labels as there "
                  << "are transparent copies)." << endl;
        exit(1);
    }
    uint u;
    if (layout_labels.empty()) {
        for (u = 0; u < labels.size(); u++) {
            layout_labels.push_back(std::vector<std::string>());
        }
    }
    for (u = 0; u < labels.size(); u++) {
        layout_labels[u].push_back(labels[u]);
    }
}

void DCFilterInstance::bind_to_cluster(std::vector<std::string> hosts)
{
    composite_cluster_binding = hosts;
}

void DCFilterInstance::designate_as_inbound_for_composite() {
    this->is_inbound_composite_designate = true;
}

void DCFilterInstance::designate_as_outbound_for_composite() {
    this->is_outbound_composite_designate = true;
}

void DCFilterInstance::add_port(std::string local_port_name,
                                DCFilterInstance * remote_filter_instance,
                                std::string remote_port_name)
{
    if (this->outports.count(local_port_name) > 0) {
        outports[local_port_name].add_remote(
            remote_filter_instance, remote_port_name);
    }
    else {
        outports[local_port_name] =
            MultiPort(remote_filter_instance, remote_port_name);
    }
}
