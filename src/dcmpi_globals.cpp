#ifdef DCMPI_HAVE_PYTHON
#include <Python.h>
#endif

#include "dcmpi_internal.h"

#include "dcmpi.h"

#include "DCMutex.h"
#include "MemUsage.h"

#include "dcmpi_globals.h"

#ifdef DCMPI_HAVE_JAVA
#include <jni.h>
#endif

#include <libgen.h>

#ifdef DCMPI_HAVE_LZOP
#include <lzo/lzo1x.h>
#endif

using namespace std;

MUID MUID_of_dcmpiruntime = -1; // invalid until later
MemUsage ** MUID_MemUsage = NULL; // sparse array mapping MUID's to MemUsage
MemUsage local_console_outgoing_memusage(MB_64, "\"local console filter\"");
int dcmpi_num_total_filters = -1;
bool dcmpi_is_local_console = true;
bool dcmpi_remote_console_snarfed_layout = false;
std::string dcmpi_remote_argv_0;

#ifdef DCMPI_HAVE_JAVA
JavaVM * dcmpi_jni_invocation_jvm = NULL;
JNIEnv * dcmpi_jni_mainthread_env = NULL;
DCMutex  dcmpi_jni_mainthread_env_mutex;
int      dcmpi_jni_users          = 0;
#endif

void init_context_free_globals();
void fini_context_free_globals();
class global_initializer
{
public:
    global_initializer()
    {
        init_context_free_globals();
    }
};
static global_initializer gi;

#ifdef DCMPI_HAVE_JAVA
void init_java() // this is always called when a mutex is locked
{
    if (dcmpi_jni_invocation_jvm) {
        return;
    }
    jsize existing;
    if (JNI_GetCreatedJavaVMs(&dcmpi_jni_invocation_jvm,1,&existing)!=0) {
        std::cerr << "ERROR: calling JNI_GetCreatedJavaVMs()"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    if (existing>0) {
        assert(dcmpi_is_local_console);
        assert(existing==1);
        jint res = dcmpi_jni_invocation_jvm->AttachCurrentThread(
            (void**)&dcmpi_jni_mainthread_env,
            NULL);
        if (res < 0) {
            std::cerr << "ERROR: cannot attach to existing java VM"
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }        
        return;
    }
    dcmpi_jni_invocation_jvm=NULL;
    std::string dcmpi_runtime_dir;
    if (dcmpi_is_local_console) {
        dcmpi_runtime_dir = dcmpi_file_dirname(
            dcmpi_find_executable("dcmpiconfig"));
    }
    else {
        dcmpi_runtime_dir = dcmpi_file_dirname(dcmpi_remote_argv_0);
    }

    std::string s = dcmpi_file_basename(dcmpi_runtime_dir);
    bool installtree = false;
    if (s == "src") {
        ;
    }
    else if (s == "bin") {
        installtree = true;
    }
    else {
        std::cerr << "ERROR: dcmpiconfig must be located in a src/ "
                  << "or bin/ directory!"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }

    JavaVMInitArgs vm_args;
    memset(&vm_args, 0, sizeof(vm_args));
    vm_args.nOptions = 0;
#define MAX_OPTIONS 10
    JavaVMOption * options = new JavaVMOption[MAX_OPTIONS];
    memset(options, 0, sizeof(JavaVMOption)*MAX_OPTIONS);
    int next_option_index = 0;
    std::string classpath_key = "-Djava.class.path=";
    std::string classpath_val;

    // find dcmpi_java_support.jar
    std::string dcmpi_java_support_jar;
    if (!installtree) {
        dcmpi_java_support_jar = dcmpi_runtime_dir;
        dcmpi_java_support_jar += "/java/dcmpi_java_support.jar";
    }
    else {
        dcmpi_java_support_jar = dcmpi_file_dirname(dcmpi_runtime_dir)
            + "/lib/dcmpi_java_support.jar";
    }
    if (classpath_val.size()) {
        classpath_val += ":";
    }
    classpath_val += dcmpi_java_support_jar;

    if (getenv("DCMPI_JAVA_CLASSPATH")) {
        if (classpath_val.size() &&
            classpath_val[classpath_val.size()-1] != ':') {
            classpath_val += ":";
        }
        classpath_val += getenv("DCMPI_JAVA_CLASSPATH");
    }

    std::vector<std::string> config_files =
        get_config_filenames("dcmpi_java_classpath");
    for (int i = 1; i >= 0; i--) {
        if (config_files[i] != "") {
            std::vector<std::string> lines =
                get_config_noncomment_nonempty_lines(config_files[i]);
            for (uint u2 = 0; u2 < lines.size(); u2++) {
                std::string line = lines[u2];
                std::vector<std::string> toks = dcmpi_string_tokenize(line, ":");
                for (uint u3 = 0; u3 < toks.size(); u3++) {
                    std::string term = toks[u3];
                    if (dcmpi_file_exists(term)) {
                        if (classpath_val.size() &&
                            classpath_val[classpath_val.size()-1] != ':') {
                            classpath_val += ":";
                        }
                        classpath_val += term;
                    }
                }
            }
        }
    }
    
    if (getenv("CLASSPATH")) {
        if (classpath_val.size() &&
            classpath_val[classpath_val.size()-1] != ':') {
            classpath_val += ":";
        }
        classpath_val += getenv("CLASSPATH");
    }
    if (classpath_val.size()) {
        std::string classpath = classpath_key + classpath_val;
        options[next_option_index].optionString = StrDup(classpath.c_str());
        options[next_option_index++].extraInfo = NULL;
    }

    // bootstrap to JNI C libraries on remote side
    if (!dcmpi_is_local_console) {
        std::string libpath;
        libpath = "-Djava.library.path=";
        if (!installtree) {
            s = dcmpi_runtime_dir + "/java";
        }
        else {
            s = dcmpi_file_dirname(dcmpi_runtime_dir) + "/lib";
        }
        libpath += s;
        options[next_option_index].optionString = StrDup(libpath.c_str());
        options[next_option_index++].extraInfo = NULL;
    }

//     options[next_option_index].optionString = StrDup("-verbose:jni");
//     options[next_option_index++].extraInfo = NULL;

    options[next_option_index].optionString = StrDup("-Xcheck:jni");
    options[next_option_index++].extraInfo = NULL;

    if (!dcmpi_is_local_console && getenv("DCMPI_JAVA_MINIMUM_HEAP_SIZE")) {
        std::string argument = "-Xms";
        argument += getenv("DCMPI_JAVA_MINIMUM_HEAP_SIZE");
        options[next_option_index].optionString = StrDup(argument.c_str());
        options[next_option_index++].extraInfo = NULL;
    }
    else {   
        options[next_option_index].optionString = StrDup("-Xms64m");
        options[next_option_index++].extraInfo = NULL;
    }
    
    if (!dcmpi_is_local_console && getenv("DCMPI_JAVA_MAXIMUM_HEAP_SIZE")) {
        std::string argument = "-Xmx";
        argument += getenv("DCMPI_JAVA_MAXIMUM_HEAP_SIZE");
        options[next_option_index].optionString = StrDup(argument.c_str());
        options[next_option_index++].extraInfo = NULL;
    }
    else {   
        options[next_option_index].optionString = StrDup("-Xmx256m");
        options[next_option_index++].extraInfo = NULL;
    }

    if (!dcmpi_is_local_console && (getenv("DCMPI_JAVA_EXTRA_STARTUP_ARGS"))) {
        std::vector<std::string> tokens =
            dcmpi_string_tokenize(getenv("DCMPI_JAVA_EXTRA_STARTUP_ARGS"));
        for (uint u = 0; u < tokens.size(); u++) {
            dcmpi_string_trim(tokens[u]);
            options[next_option_index].optionString =
                StrDup(tokens[u].c_str());
            options[next_option_index++].extraInfo = NULL;
        }
    }

    if (!dcmpi_is_local_console && getenv("DCMPI_JAVA_REMOTE_DEBUG_PORT")) {
        std::string s = "-agentlib:jdwp=transport=dt_socket,address=";
        s += getenv("DCMPI_JAVA_REMOTE_DEBUG_PORT");
        s += ",server=y,suspend=n";
        options[next_option_index].optionString = StrDup(s.c_str());
        options[next_option_index++].extraInfo = NULL;
    }

    vm_args.options = options;
    vm_args.nOptions = next_option_index;
    vm_args.version = JNI_VERSION_1_4;
    vm_args.ignoreUnrecognized = JNI_FALSE;
    if (dcmpi_verbose()) {
        for (int i = 0; i < next_option_index; i++) {
            std::cout << "JNI on "
                      << dcmpi_get_hostname()
                      << " invoked with option " << i
                      << " set to "
                      << options[i].optionString << endl;
        }
    }
#ifdef DCMPI_HAVE_MACOSX
    setenv("JAVA_JVM_VERSION", "1.4*", 1);
#endif
    jint res = JNI_CreateJavaVM(&dcmpi_jni_invocation_jvm,
                                (void**)&dcmpi_jni_mainthread_env,
                                &vm_args);
    if (res < 0) {
        std::cerr << "ERROR: cannot create java VM"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    if (vm_args.nOptions > 0) {
        for (int i = 0; i < vm_args.nOptions; i++) {
            delete[] options[i].optionString;
        }
        delete[] options;
    }
    if (dcmpi_is_local_console) {
        atexit(fini_java);
    }
}

void fini_java()
{
    if (dcmpi_jni_invocation_jvm) {
      if (dcmpi_verbose()) {
	cout << "Destroying JVM at " << __FILE__ << ":" << __LINE__ << endl;
	cout << "Actually we are faking it.. DestroyJVM causes hanging in certain situations.. but since our application is going to exit, it will clean up the jvm" << endl;
      }


      /** commented out by k2 because this causes hanging */
      //dcmpi_jni_invocation_jvm->DestroyJavaVM();
        dcmpi_jni_invocation_jvm = NULL;

      if (dcmpi_verbose()) {
	cout << "Destroyed JVM at " << __FILE__ << ":" << __LINE__ << endl;
      }

    }
}
#endif

#ifdef DCMPI_HAVE_PYTHON
PyThreadState * dcmpi_pyMainThreadState = NULL;
DCMutex dcmpi_pymainthread_mutex;

void init_python(void) // this is always called when a mutex is locked
{
    if (dcmpi_pyMainThreadState) {
        return;
    }
    std::string dcmpi_runtime_dir;
    if (dcmpi_is_local_console) {
        dcmpi_runtime_dir = dcmpi_file_dirname(
            dcmpi_find_executable("dcmpiconfig"));
    }
    else {
        dcmpi_runtime_dir = dcmpi_file_dirname(dcmpi_remote_argv_0);
    }

    std::string s = dcmpi_file_basename(dcmpi_runtime_dir);
    bool installtree = false;
    if (s == "src") {
        ;
    }
    else if (s == "bin") {
        installtree = true;
    }
    else {
        std::cerr << "ERROR: dcmpiconfig must be located in a src/ "
                  << "or bin/ directory!"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    std::string added;
    if (installtree) {
        s = dcmpi_file_dirname(dcmpi_runtime_dir);
        s += "/lib";
        added = s;
    }
    else {
        added = dcmpi_runtime_dir;
    }

    if (getenv("DCMPI_PYTHONPATH")) {
        added += ":";
        added += getenv("DCMPI_PYTHONPATH");
        if (dcmpi_verbose()) {
            std::cout << "adding "
                      << getenv("DCMPI_PYTHONPATH")
                      << " to PYTHONPATH (from DCMPI_PYTHONPATH)\n";
        }
    }
    
    std::vector<std::string> config_files =
        get_config_filenames("dcmpi_pythonpath");
    for (int i = 1; i >= 0; i--) {
        if (config_files[i] != "") {
            std::vector<std::string> lines =
                get_config_noncomment_nonempty_lines(config_files[i]);
            for (uint u2 = 0; u2 < lines.size(); u2++) {
                std::string line = lines[u2];
                std::vector<std::string> toks = dcmpi_string_tokenize(line, ":");
                for (uint u3 = 0; u3 < toks.size(); u3++) {
                    std::string term = toks[u3];
                    if (dcmpi_file_exists(term)) {
                        added += ":";
                        added += term;
                        if (dcmpi_verbose()) {
                            std::cout << "dcmpi_pythonpath: adding "
                                      << term << "\n";
                        }
                    }
                }
            }
        }
    }

    std::string newval;
    if (getenv("PYTHONPATH")) {
        std::string existing = getenv("PYTHONPATH");
        newval = added + ":" + getenv("PYTHONPATH");
    }
    else {
        newval = added;
    }
    setenv("PYTHONPATH", newval.c_str(), 1);

#ifndef DEBUG
    Py_OptimizeFlag = 1; // generate .pyo instead of .pyc files
#endif
    
    Py_Initialize();
    PyEval_InitThreads(); // acquires lock
    dcmpi_pyMainThreadState = PyThreadState_Get();
    if (dcmpi_is_local_console) {
        atexit(fini_python);
    }
    PyEval_ReleaseLock();
}
void fini_python(void)
{
    if (dcmpi_pyMainThreadState) {
        PyEval_AcquireLock();
        PyThreadState_Swap(dcmpi_pyMainThreadState);
        Py_Finalize();
        dcmpi_pyMainThreadState = NULL;
    }
}
#endif

void init_context_free_globals()
{
#ifdef DCMPI_HAVE_LZOP
    lzo_init();
#endif
}

void fini_context_free_globals()
{
    ;
}

void init_dcmpiruntime_globals(int numfilters)
{
    dcmpi_num_total_filters = numfilters;

    MUID_MemUsage = new MemUsage*[numfilters+1];
    for (int i = 0; i < numfilters+1; i++) {
        MUID_MemUsage[i] = NULL;
    }
    MUID_of_dcmpiruntime = numfilters;
//     MUID_of_console_buffer_area = numfilters+1;
}

void fini_dcmpiruntime_globals()
{
    for (int i = 0; i < dcmpi_num_total_filters+1; i++) {
        delete MUID_MemUsage[i];
    }
    delete[] MUID_MemUsage;
}
