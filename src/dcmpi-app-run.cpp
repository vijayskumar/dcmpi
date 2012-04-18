#include "dcmpi_util.h"
#include "dcmpisqlite3.h"
#include "dcmpi-configdb.h"

using namespace std;

static int getapps_1(void *Used, int argc, char **argv, char **azColName){
    std::vector<DCApp> * apps = (std::vector<DCApp>*)Used;
    DCApp app;
    app.name = argv[0];
    app.executable = argv[1];
    app.description = argv[2];
    apps->push_back(app);
    return 0;
}

static int getapps_2(void *Used, int argc, char **argv, char **azColName) {
    DCApp * app = (DCApp*)Used;
    DCAppParam p;
    p.varname=argv[0];
    p.description=argv[1];
    p.required=(string(argv[2])=="y");
    p.maxoccur=atoi(argv[3]);
    p.minoccur=atoi(argv[4]);
    if (string(argv[5])=="I") {
        app->inputs.push_back(p);
    }
    else {
        app->outputs.push_back(p);
    }
    return 0;
}

std::vector<DCApp> getapps_internal(sqlite3 * db)
{
    char * sql =
        "select r.name, r.executable, r.description  "
        "from registered_apps r "
        "order by r.name";
    char * zErrMsg = NULL;
    std::vector<DCApp> apps;
    if (sqlite3_exec(db, sql, getapps_1, &apps, &zErrMsg)) {
        std::cerr << "ERROR: " << zErrMsg
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    uint u;
    for (u = 0; u < apps.size(); u++) {
        std::string sql =
            "select a.varname, a.description, a.required, a.maxoccur, a.minoccur, a.IO "
            "from app_IO a "
            "where a.appname ='";
        sql += sql_escape(apps[u].name) + "' order by a.varname";
        char * zErrMsg = NULL;
        if (sqlite3_exec(db, sql.c_str(), getapps_2, &apps[u], &zErrMsg)) {
            std::cerr << "ERROR: " << zErrMsg
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
    }
    return apps;
}

std::vector<DCApp> getapps(void)
{
    uint u;
    std::vector<DCApp> apps;
    if (db_systemwide_config) {
        apps = getapps_internal(db_systemwide_config);
    }
    if (db_user_config) {
        std::vector<DCApp> appsuser = getapps_internal(db_user_config);
        for (u = 0; u < appsuser.size(); u++) {
            uint u2;
            int replaced = 0;
            for (u2 = 0; u2 < apps.size(); u2++) {
                if (apps[u2].name == appsuser[u].name) {
                    apps[u2] = appsuser[u];
                    replaced = 1;
                    break;
                }
            }
            if (!replaced) {
                apps.push_back(appsuser[u]);
            }
        }
    }
    return apps;
}

std::vector<std::string> getappnames()
{
    std::vector<DCApp> apps = getapps();
    std::vector<std::string> names;
    for (uint u = 0; u < apps.size(); u++) {
        names.push_back(apps[u].name);
    }
    return names;
}

char * appname = NULL;
void usage()
{
    printf("usage: %s <app> <inputs_serialized_file> <outputs_serialized_file>\n", appname);
    exit(EXIT_FAILURE);
}

void err(char * outfile, std::string message)
{
    cerr << "dcmpi-app-run: " << message << endl;
    FILE * f;
    if ((f = fopen(outfile, "w")) == NULL) {
        std::cerr << "ERROR: errno=" << errno << " opening file"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    message = tostr("status\n") + tostr(message.size()) + "\n" + message + "\n";
    if (fwrite(message.c_str(), message.size(), 1, f) < 1) {
        std::cerr << "ERROR: errno=" << errno << " calling fwrite()"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    if (fclose(f) != 0) {
        std::cerr << "ERROR: errno=" << errno << " calling fclose()"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
}

int main(int argc, char * argv[])
{
    appname = argv[0];
    bool debug=false;
    while (argc > 1) {
        if (!strcmp(argv[1], "-h")) {
            usage();
        }
        else if (!strcmp(argv[1], "-d")) {
            debug=true;
        }
        else {
            break;
        }
        dcmpi_args_shift(argc, argv);
    }

    if ((argc-1) != 3) {
        usage();
    }
    std::string appname = argv[1];
    std::string input_ser = argv[2];
    std::string output_ser = argv[3];
    dcmpi_configdb_init();
    std::vector<DCApp> apps = getapps();
    std::vector<std::string> appnames = getappnames();
    if (std::find(appnames.begin(),
                  appnames.end(),
                  appname) == appnames.end()) {
        ostr o;
        o << "ERROR: unknown app " << appname;
        err(argv[3], o.str());
        exit(1);
    }

    // validate args
    FILE * fin;
    if ((fin = fopen(argv[2], "r")) == NULL) {
        ostr o;
        o << "ERROR: errno=" << errno << " opening input file " << argv[2]
          << " at " << __FILE__ << ":" << __LINE__;
        err(argv[3], o.str());
        exit(1);
    }
    std::set<std::string> inparams;
    while (1) {
        std::string line = dcmpi_file_read_line(fin);
        if (line.size()==0) {
            break;
        }
        dcmpi_string_trim(line);
        inparams.insert(line);
        line = dcmpi_file_read_line(fin);
        dcmpi_string_trim(line);
        int8 b = Atoi8(line);
        if (fseeko(fin, b, SEEK_CUR) != 0) {
            err(argv[3], "reading input file");
        }
        char newline;
        if (fread(&newline, 1, 1, fin) < 1) {
            err(argv[3], "no trailing newline");
            exit(1);
        }
    }
    if (fclose(fin) != 0) {
        std::cerr << "ERROR: errno=" << errno << " calling fclose()"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    
    uint u;
    DCApp app;
    for (u = 0; u < apps.size(); u++) {
        if (apps[u].name == appname) {
            app = apps[u];
            break;
        }
    }

    for (u = 0; u < app.inputs.size(); u++) {
        std::string & input = app.inputs[u].varname;
        if (inparams.find(input)==inparams.end()) {
            ostr o;
            o << "ERROR: required input parameter " << input
              << " was not specified.";
            err(argv[3], o.str());
            exit(1);
        }
    }
    if (debug) {
        std::cout << "+" << app.executable
                  << " " << input_ser << " "
                  << output_ser << endl;
    }
    dcmpi_configdb_fini();
    execlp(app.executable.c_str(), app.executable.c_str(),
           input_ser.c_str(), output_ser.c_str(), (char*)NULL);
}

