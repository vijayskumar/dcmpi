/***************************************************************************
    $Id: Instance_Port.cpp,v 1.3 2006/10/31 10:02:00 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

using namespace std;

#include "Instance_Port.h"

bool Instance_Port::operator<(const Instance_Port & i) const
{
    return (instance->instancename < i.instance->instancename) ||
        ((instance->instancename < i.instance->instancename) &&
         (port < i.port));
}

bool Instance_Port::operator==(const Instance_Port & i) const
{
    return ((instance->instancename == i.instance->instancename) &&
            (port == i.port));
}

std::ostream& operator<<(std::ostream &o, const Instance_Port & i)
{
    return o << i.instance->get_distinguished_name() << "[" << i.port << "]";
}
