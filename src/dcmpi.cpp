#include "dcmpi_clib.h"

#include "dcmpi.h"
#include "DCMutex.h"
#include "dcmpi_util.h"
#include "dcmpi_sha1.h"

using namespace std;

#ifdef DCMPI_HAVE_RAND_R
int dcmpi_rand()
{
    int out_value;
    static unsigned int dcmpi_rand_state;
    static DCMutex m;
    static bool inited = false;
    m.acquire();
    if (!inited) {
        double f = dcmpi_doubletime();
        int i = (int)((f - (int)f) * 1000000);
        dcmpi_rand_state = i;
        inited = true;
    }
    out_value = rand_r(&dcmpi_rand_state);
    m.release();
    return out_value;
}
#else
int dcmpi_rand()
{
    int out_value;
    static DCMutex m;
    static bool inited = false;
    m.acquire();
    if (!inited) {
        double f = dcmpi_doubletime();
        int i = (int)((f - (int)f) * 1000000);
        srand(i);
        inited = true;
    }
    out_value = rand();
    m.release();
    return out_value;
}
#endif

bool dcmpi_verbose()
{
    return getenv("DCMPI_VERBOSE") != NULL;
}

std::string dcmpi_find_executable(const std::string & name,
                                  bool fail_on_missing)
{
    std::vector<std::string> tokens = dcmpi_string_tokenize(getenv("PATH"), ":");
    uint u;
    for (u = 0; u < tokens.size(); u++) {
        std::string component = tokens[u];
        std::string fn = component + "/";
        fn += name;
        if (fileExists(fn)) {
            return fn;
        }
    }
    if (fail_on_missing) {
        std::cerr << "ERROR: cannot find executable " << name
                  << "...is it in your PATH?"
                  << std::endl << std::flush;
        exit(1);
    }
    return name; // safe fallback
}

std::string dcmpi_get_temp_filename()
{
    static DCMutex m;
    char tpl[PATH_MAX];
    sprintf(tpl, "/tmp/DCMPI_XXXXXX");
    int fd;
    m.acquire(); // protect umask
    mode_t prev_mask = umask(0077); // some versions of mkstemp create with
                                    // mode 0666, so this is necessary

    fd = mkstemp(tpl);
    if (fd == -1) {
        std::cerr << "ERROR: mkstemp() failed"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    umask(prev_mask);
    m.release();
    close(fd);
    std::string out = tpl;
    return out;
}

std::string dcmpi_get_temp_dir()
{
    static DCMutex m;
    char tpl[PATH_MAX];
    sprintf(tpl, "/tmp/DCMPID_XXXXXX");
    m.acquire(); // protect umask
    mode_t prev_mask = umask(0077); // to be safe
    if (mkdtemp(tpl) == NULL) {
        std::cerr << "ERROR: mkdtemp() failed"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    umask(prev_mask);
    m.release();
    std::string out = tpl;
    return out;
}

void dcmpi_args_shift(int & argc, char ** argv, int frompos)
{
    int i;
    for (i = frompos; i < (argc-1); i++) {
        argv[i] = argv[i+1];
    }
    argv[argc-1] = NULL;
    argc--;
}

void dcmpi_args_pushback(int & argc, char ** & argv, const char * addme)
{
    // better reallocate argv so we can write to the end
    char ** new_argv = new char*[argc+1];
    int i;
    for (i = 0; i < argc; i++) {
        new_argv[i] = argv[i];
    }
    new_argv[i] = strdup(addme); // yes, this is a memory leak but there is no
                                 // better solution other than some atexit()
                                 // nonsense
    argc++;
    argv = new_argv;
}

std::string dcmpi_get_time()
{
    time_t                  the_time;
    struct tm *             the_time_tm;
    std::string out;
    static DCMutex mutex;

    mutex.acquire();
    the_time = time(NULL);
    the_time_tm = localtime(&the_time);
    out = asctime(the_time_tm);
    if (out[out.length()-1] == '\n') {
        out.erase(out.length()-1, 1);
    }
    mutex.release();
    return out;
}

void dcmpi_string_trim_front(std::string & s)
{
    s.erase(0,s.find_first_not_of(" \t\n"));
}

void dcmpi_string_trim_rear(std::string & s)
{
    s.erase(s.find_last_not_of(" \t\n")+1);
}

void dcmpi_string_trim(std::string & s)
{
    dcmpi_string_trim_front(s);
    dcmpi_string_trim_rear(s);
}

void dcmpi_doublesleep(double secs)
{
    double frac;
	frac = secs - (int)secs;
    sleep((int)secs);
    usleep((long)(frac * 1000000.0));
}

double dcmpi_doubletime()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return (double) tp.tv_sec + (double) tp.tv_usec / 1000000.0;
}

unsigned long long dcmpi_csnum(const std::string & num)
{
    std::string num_part(num);
    int lastIdx = num.size() - 1;
    char lastChar = num[lastIdx];
    unsigned long long multiplier = 1;
    unsigned long long kb_1 = (unsigned long long)1024;
    if ((lastChar == 'k') || (lastChar == 'K')) {
        multiplier = kb_1;
    }
    else if ((lastChar == 'm') || (lastChar == 'M')) {
        multiplier = kb_1*kb_1;
    }
    else if ((lastChar == 'g') || (lastChar == 'G')) {
        multiplier = kb_1*kb_1*kb_1;
    }
    else if ((lastChar == 't') || (lastChar == 'T')) {
        multiplier = kb_1*kb_1*kb_1*kb_1;
    }
    else if ((lastChar == 'p') || (lastChar == 'P')) {
        multiplier = kb_1*kb_1*kb_1*kb_1*kb_1;
    }

    if (multiplier != 1) {
        num_part.erase(lastIdx, 1);
    }
    unsigned long long n = strtoull(num_part.c_str(), NULL, 10);
    return n * multiplier;
}

int dcmpi_file_lines(const std::string & filename)
{
    char buf[256];
    FILE * f = fopen(filename.c_str(), "r");
    assert(f);
    int lines = 0;
    while (fgets(buf, sizeof(buf), f) != NULL) {
        lines++;
    }
    fclose(f);
    return lines;
}

std::vector<std::string> dcmpi_file_lines_to_vector(
    const std::string & filename)
{
    std::vector<std::string> out;
    char buf[256];
    FILE * f = fopen(filename.c_str(), "r");
    if (!f) {
        std::cerr << "ERROR: opening file " << filename
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    while (fgets(buf, sizeof(buf), f) != NULL) {
        buf[strlen(buf)-1] = 0;
        if ((strlen(buf) > 0) &&
            buf[0] != '#')
            out.push_back(buf);
    }
    fclose(f);
    return out;
}

bool dcmpi_file_exists(const std::string & filename)
{
    /* stat returns 0 if the file exists */
    struct stat stat_out;
    return (stat(filename.c_str(), &stat_out) == 0);
}

std::string dcmpi_file_dirname(const std::string & filename)
{
    char fn[PATH_MAX];
    strcpy(fn, filename.c_str());
    std::string out = dirname(fn);
    return out;
}
 
std::string dcmpi_file_basename(const std::string & filename)
{
    char fn[PATH_MAX];
    strcpy(fn, filename.c_str());
    std::string out = basename(fn);
    return out;
}

int dcmpi_mkdir_recursive(const std::string & dir)
{
    std::string dirStr = dir;
    int rc = 0;
    std::vector<std::string> components = dcmpi_string_tokenize(dirStr, "/");
    dirStr = "";
    if (dir[0] == '/') {
        dirStr += "/";
    }
    
    for (unsigned loop = 0, len = components.size(); loop < len; loop++) {
        dirStr += components[loop];
        if (!dcmpi_file_exists(dirStr)) {
            if (mkdir(dirStr.c_str(), 0777) == -1) {
                rc = errno;
                goto Exit;
            }
        }
        dirStr += "/";
    }

Exit: 
    return rc;
}

int dcmpi_rmdir_recursive(const std::string & dir)
{
    if (!dcmpi_file_exists("/bin/rm")) {
        std::cerr << "ERROR: cannot find /bin/rm"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        return -1;
    }
    if (dcmpi_file_exists(dir)) {
        std::string cmd = "/bin/rm -fr ";
        std::string d = dir;
        dcmpi_string_replace(d, " ", "\\ ");
        cmd += d;
        int rc = system(cmd.c_str());
        if (rc) {
            std::cerr << "ERROR: running command "
                      << cmd
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            return -1;
        }
    }
    return 0;
}

std::string dcmpi_get_hostname(bool force_short_name)
{
    std::string out;
    char localhost[1024];
    gethostname(localhost, sizeof(localhost));
    localhost[sizeof(localhost)-1] = 0;
    out = localhost;
    if (force_short_name) {
        dcmpi_hostname_to_shortname(out);
    }
    return out;
}

void dcmpi_gethostbyname(const std::string & name,
                         std::string & out_name,
                         std::vector<std::string> & out_aliases)
{
    out_aliases.clear();
    struct hostent * v;
#ifdef DCMPI_HAVE_GETHOSTBYNAME_R
    struct hostent ret;
    char buf[2048];
    int err;
    if (gethostbyname_r(name.c_str(), &ret, buf, sizeof(buf), &v, &err)
        != 0) {
        out_name = name;
        return;
    }
#else
    v = gethostbyname(name.c_str());
#endif
    if (!v) {
        out_name = name;
    }
    else {
        out_name = v->h_name;
        int i = 0;
        while (v->h_aliases[i]) {
            out_aliases.push_back(v->h_aliases[i]);
            i += 1;
        }
    }
}

void dcmpi_hostname_to_shortname(std::string & hostname)
{
    std::string::size_type dotpos = hostname.find(".");
    if (dotpos != std::string::npos) {
        hostname.erase(dotpos, hostname.size() - dotpos);    
    }
}

void dcmpi_sha1(void * message, size_t message_len, unsigned char digest[20])
{
    sha1_context context;
    sha1_starts(&context);
    sha1_update(&context, (uint1*)message, (uint4)message_len);
    sha1_finish(&context, digest);
}
        
std::string dcmpi_sha1_tostring(void * message, size_t message_len)
{
    unsigned char digest[20];
    dcmpi_sha1(message, message_len, digest);
    char s[64];
    s[0] = 0;
    char s1[8];
    for (int i = 0; i < 20; i++) {
        sprintf(s1, "%x", (unsigned int)digest[i]);
        if (strlen(s1) == 1) {
            strcat(s, "0");
        }
        else {
            assert(strlen(s1) == 2);
        }
        strcat(s, s1);
    }
    std::string out = s;
    return out;
}
