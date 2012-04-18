#ifndef RESOLVEDMULTIPORT_H
#define RESOLVEDMULTIPORT_H

/***************************************************************************
    $Id: ResolvedMultiPort.h,v 1.3 2005/12/13 13:58:44 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include "dcmpi.h"

#include "Gftid_Port.h"

/*  after resolved to gftids */
class ResolvedMultiPort : public DCSerializable
{
public:
    std::vector<Gftid_Port> remotes;
    void add_remote(int remote_gftid, std::string remote_port_name)
    {
        this->remotes.push_back(Gftid_Port(remote_gftid, remote_port_name));
    }
    friend std::ostream& operator<<(std::ostream &o,
                                    const ResolvedMultiPort & i);
    void serialize(DCBuffer * buf) const
    {
        uint u;
        buf->Append((int4)remotes.size());
        for (u = 0; u < remotes.size(); u++) {
            remotes[u].serialize(buf);
        }
    }
    void deSerialize(DCBuffer * buf)
    {
        int4 sz;
        int4 i;
        remotes.clear();
        buf->Extract(&sz);
        for (i = 0; i < sz; i++) {
            Gftid_Port gp;
            gp.deSerialize(buf);
            remotes.push_back(gp);
        }
    }
};

#endif /* #ifndef RESOLVEDMULTIPORT_H */
