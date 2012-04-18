#ifndef GFTID_PORT_H
#define GFTID_PORT_H

/***************************************************************************
    $Id: Gftid_Port.h,v 1.4 2005/12/13 13:58:44 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include <string>

#include "dcmpi.h"

// GFTID stands for "Global Filter Thread ID"

class Gftid_Port : public DCSerializable
{
public:
    int gftid;
    std::string port;
    Gftid_Port(int gftid, std::string port)
    {
        this->gftid = gftid;
        this->port = port;
    }
    friend std::ostream& operator<<(std::ostream &o, const Gftid_Port& i);
    bool operator==(const Gftid_Port & other) const
    {
        return (this->gftid == other.gftid) && (this->port == other.port);
    }
    bool operator<(const Gftid_Port & other) const
    {
        return ((this->gftid < other.gftid) || 
                ((this->gftid == other.gftid) && (this->port < other.port)));
    }
    void serialize(DCBuffer * buf) const
    {
        buf->Append((int4)gftid);
        buf->Append(port);
    }
    void deSerialize(DCBuffer * buf)
    {
        int4 i;
        buf->Extract(&i);
        gftid = i;
        buf->Extract(&port);
    }
private:
    Gftid_Port(){} friend class ResolvedMultiPort;friend class DCFilterInstance;
};

#endif /* #ifndef GFTID_PORT_H */
