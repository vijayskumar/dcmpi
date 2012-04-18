#ifndef INSTANCE_PORT_H
#define INSTANCE_PORT_H

/***************************************************************************
    $Id: Instance_Port.h,v 1.3 2006/10/31 10:02:00 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include "dcmpi_clib.h"

#include "dcmpi.h"

class Instance_Port
{
public:
    DCFilterInstance * instance;
    std::string port;
    bool operator<(const Instance_Port & i) const;
    bool operator==(const Instance_Port & i) const;
    Instance_Port(DCFilterInstance * instance, std::string port)
    {
        this->instance = instance;
        this->port = port;
    }
    friend std::ostream& operator<<(std::ostream &o, const Instance_Port & i);
};

#endif /* #ifndef INSTANCE_PORT_H */
