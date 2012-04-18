#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <queue>
#include <vector>

struct sqlite3;

extern sqlite3 * db_systemwide_config;
extern sqlite3 * db_user_config;
void dcmpi_configdb_init(void);
void dcmpi_configdb_fini(void); // doesn't need to be called under normal
                                // termination (exit(), return from main)

class DCAppParam
{
public:
    std::string varname;
    std::string description;
    bool required;
    int minoccur;
    int maxoccur;
};
inline std::ostream& operator<<(std::ostream &o, const DCAppParam & i)
{
    o << "             name: " << i.varname << "\n             desc: " << i.description
      << "\n             required: ";
    if (i.required) {
        o << "y";
    }
    else {
        o << "n";
    }
    o << " min: " << i.minoccur << " max: " << i.maxoccur;
    return o;
}

class DCApp
{
public:
    std::string name;
    std::string executable;
    std::string description;
    std::vector<DCAppParam> inputs;
    std::vector<DCAppParam> outputs;
    bool has_input(std::string s)
    {
        uint u;
        for (u = 0; u < inputs.size(); u++) {
            if (inputs[u].varname==s) {
                return true;
            }
        }
        return false;
    }
    bool has_output(std::string s)
    {
        uint u;
        for (u = 0; u < outputs.size(); u++) {
            if (outputs[u].varname==s) {
                return true;
            }
        }
        return false;
    }
};
inline std::ostream& operator<<(std::ostream &o, const DCApp & i)
{
    o << "   app name: " << i.name << std::endl
      << " executable: " << i.executable << std::endl
      << "description: " << i.description << std::endl;
    o << "     inputs:\n";
    for (uint u = 0; u < i.inputs.size(); u++) {
        o << i.inputs[u] << std::endl << std::endl;
    }
    o << "    outputs:\n";
    for (uint u = 0; u < i.outputs.size(); u++) {
        o << i.outputs[u] << std::endl << std::endl;
    }
    return o;
}
