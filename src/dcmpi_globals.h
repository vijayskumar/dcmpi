#ifndef DCMPI_GLOBALS_H
#define DCMPI_GLOBALS_H

#ifdef DCMPI_HAVE_PYTHON
#include <Python.h>
#endif

#include "DCMutex.h"

class MemUsage;

void init_dcmpiruntime_globals(int numfilters);
void fini_dcmpiruntime_globals();

// MUID numbers are same as gftid, except 'dcmpiruntime' buffer area id is 1 +
// #filters
typedef int MUID; // Memory Usage ID
extern MUID MUID_of_dcmpiruntime;
extern MemUsage local_console_outgoing_memusage;
extern MemUsage ** MUID_MemUsage; // sparse array mapping array index MUID's
                                  // to MemUsage* objects

extern int dcmpi_num_total_filters;
extern bool dcmpi_is_local_console;
extern bool dcmpi_remote_console_snarfed_layout;
extern std::string dcmpi_remote_argv_0;

#ifdef DCMPI_HAVE_JAVA
#include <jni.h>
extern JavaVM * dcmpi_jni_invocation_jvm;
extern JNIEnv * dcmpi_jni_mainthread_env;
extern DCMutex  dcmpi_jni_mainthread_env_mutex;

void init_java();
void fini_java();
#endif

#ifdef DCMPI_HAVE_PYTHON
extern PyThreadState * dcmpi_pyMainThreadState;
void init_python();
void fini_python();
#endif

#endif /* #ifndef DCMPI_GLOBALS_H */
