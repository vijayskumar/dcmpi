#include "dcmpi_util.h"
#include "dcmpisqlite3.h"
#include "dcmpi-configdb.h"

using namespace std;

char * appname = NULL;
void usage()
{
    printf("usage: [-systemwide] %s\n", appname);
    exit(EXIT_FAILURE);
}

sqlite3 * db;

static int getapps_1(void *Used, int argc, char **argv, char **azColName) {
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

std::vector<DCApp> getapps()
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

std::vector<std::string> getappnames()
{
    std::vector<DCApp> apps = getapps();
    std::vector<std::string> names;
    for (uint u = 0; u < apps.size(); u++) {
        names.push_back(apps[u].name);
    }
    return names;
}

void listapps(void)
{
    std::vector<DCApp> apps = getapps();
    uint u;
    for (u = 0; u < apps.size(); u++) {
        if (u) {
            std::cout << "-----------------------------------------------------------------------------\n\n";
        }
        std::cout << apps[u] << endl;
    }
}

void editapp(std::string name)
{
    uint u;

    if (name.empty()) {
        std::vector<DCApp> apps = getapps();
        if (apps.size()==0) {
            std::cerr << "ERROR: no apps registered\n";
            return;
        }
        std::vector<std::string> items;
        std::vector<std::string> out_choice;
        std::vector<int> out_choice_idx;
        for (u = 0; u < apps.size(); u++) {
            items.push_back(apps[u].name);
        }
        dcmpi_bash_select_emulator(
            2, items,
            "Please choose which app to edit:\n"
            "\n",
            "Exit",
            false, false,
            out_choice, out_choice_idx);
        if (out_choice.empty()) {
            return;
        }
        else {
            name = out_choice[0];
        }
    }

    std::vector<std::string> items;
    std::vector<std::string> out_choice;
    std::vector<int> out_choice_idx;
    items.push_back("List inputs");
    items.push_back("List outputs");
    items.push_back("Add new input");
    items.push_back("Add new output");
    items.push_back("Delete input");
    items.push_back("Delete output");
    std::string line;
    while (1) {
        std::vector<DCApp> apps = getapps();
        DCApp app;
        for (u = 0; u < apps.size(); u++) {
            if (name==apps[u].name) {
                app = apps[u];
                break;
            }
        }
        std::string desc = "Please choose one of the following actions for app ";
        desc += name + ":\n\n";
        dcmpi_bash_select_emulator(
            2, items,
            desc,
            "Exit",
            false, false,
            out_choice, out_choice_idx);
        if (out_choice.empty()) {
            break;
        }
        else if (out_choice_idx[0] == 0) {
            for (u = 0; u < app.inputs.size(); u++) {
                cout << app.inputs[u] << endl;
            }
            std::cout << std::endl;
        }
        else if (out_choice_idx[0] == 1) {
            for (u = 0; u < app.outputs.size(); u++) {
                cout << app.outputs[u] << endl;
            }
            std::cout << std::endl;
        }
        
        else if (out_choice_idx[0] == 2) {
            std::vector<std::string> input;
            printf("  Name of input: ");
            getline(cin, line);
            if (app.has_input(line)) {
                std::cerr << "ERROR: input " << line << " already exists"
                          << std::endl << std::flush;
                goto Next_step;
            }
            dcmpi_string_trim(line); input.push_back(line);
            printf("  Description of input: ");
            getline(cin, line); dcmpi_string_trim(line); input.push_back(line);
            while (1) {
                printf("  Required? (y or n): ");
                getline(cin, line); dcmpi_string_trim(line); 
                if (line=="y" or line=="n") {
                    input.push_back(line);
                    break;
                }
            }
            while (1) {
                printf("  Minimum occurences: ");
                getline(cin, line); dcmpi_string_trim(line); 
                if (dcmpi_string_isnum(line)) {
                    input.push_back(line);
                    break;
                }
            }
            while (1) {
                printf("  Maximum occurences: ");
                getline(cin, line); dcmpi_string_trim(line); 
                if (dcmpi_string_isnum(line)) {
                    input.push_back(line);
                    break;
                }
            }
            char * zErrMsg = NULL;
            std::string sql = "insert into app_IO (appname, varname, description, required, maxoccur, minoccur, IO) values (";
            sql += "'" + sql_escape(name) + "',";
            sql += "'" + sql_escape(input[0]) + "',";
            sql += "'" + sql_escape(input[1]) + "',";
            sql += "'" + sql_escape(input[2]) + "',";
            sql += "'" + sql_escape(input[3]) + "',";
            sql += "'" + sql_escape(input[4]) + "','I')";
            if (sqlite3_exec(db, sql.c_str(), NULL, NULL, &zErrMsg)) {
                std::cerr << "ERROR: " << zErrMsg
                          << " at " << __FILE__ << ":" << __LINE__
                          << std::endl << std::flush;
                exit(1);
            }
        }
        else if (out_choice_idx[0] == 3) {
            std::vector<std::string> output;
            printf("  Name of output: ");
            getline(cin, line); dcmpi_string_trim(line);
            if (app.has_output(line)) {
                std::cerr << "ERROR: output " << line << " already exists"
                          << std::endl << std::flush;
                goto Next_step;
            }
            output.push_back(line);
            printf("  Description of output: ");
            getline(cin, line); dcmpi_string_trim(line); output.push_back(line);
            while (1) {
                printf("  Required? (y or n): ");
                getline(cin, line); dcmpi_string_trim(line); 
                if (line=="y" or line=="n") {
                    output.push_back(line);
                    break;
                }
            }
            while (1) {
                printf("  Minimum occurences: ");
                getline(cin, line); dcmpi_string_trim(line); 
                if (dcmpi_string_isnum(line)) {
                    output.push_back(line);
                    break;
                }
            }
            while (1) {
                printf("  Maximum occurences: ");
                getline(cin, line); dcmpi_string_trim(line); 
                if (dcmpi_string_isnum(line)) {
                    output.push_back(line);
                    break;
                }
            }
            char * zErrMsg = NULL;
            std::string sql = "insert into app_IO (appname, varname, description, required, maxoccur, minoccur, IO) values (";
            sql += "'" + sql_escape(name) + "',";
            sql += "'" + sql_escape(output[0]) + "',";
            sql += "'" + sql_escape(output[1]) + "',";
            sql += "'" + sql_escape(output[2]) + "',";
            sql += "'" + sql_escape(output[3]) + "',";
            sql += "'" + sql_escape(output[4]) + "','O')";
            if (sqlite3_exec(db, sql.c_str(), NULL, NULL, &zErrMsg)) {
                std::cerr << "ERROR: " << zErrMsg
                          << " at " << __FILE__ << ":" << __LINE__
                          << std::endl << std::flush;
                exit(1);
            }
        }
        else if (out_choice_idx[0] == 4) {
            std::vector<std::string> items;
            std::vector<std::string> out_choice;
            std::vector<int> out_choice_idx;
            for (u = 0; u < app.inputs.size(); u++) {
                items.push_back(app.inputs[u].varname);
            }
            dcmpi_bash_select_emulator(
                2, items,
                "Select which input to delete\n",
                "Exit",
                false, false,
                out_choice, out_choice_idx);
            if (out_choice.empty()) {
                ;
            }
            else {
                std::string sql = "delete from app_IO where varname='";
                sql += out_choice[0] + "' and appname = '" + name + "' and IO='I'";
                char * zErrMsg = NULL;
                if (sqlite3_exec(db, sql.c_str(), NULL, NULL, &zErrMsg)) {
                    std::cerr << "ERROR: " << zErrMsg
                              << " at " << __FILE__ << ":" << __LINE__
                              << std::endl << std::flush;
                    exit(1);
                }
            }
        }
        else if (out_choice_idx[0] == 5) {
            ;
        }
    Next_step:
        ;
    }
}

void addapp(void)
{
    std::vector<std::string> existing = getappnames();
    std::string name;
    while (1) {
        printf("  Enter the name of your application: ");
        getline(cin, name);
        dcmpi_string_trim(name);
        if (name.find(' ')!=std::string::npos) {
            std::cerr << "  ERROR: please do not use spaces in the name\n";
        }
        else if (std::find(existing.begin(),
                           existing.end(),
                           name) != existing.end()) {
            std::cerr << "  ERROR: duplicate name " << name << endl;
        }
        else {
            break;
        }
    }

    std::string desc;
    printf("  Enter a description of your application: ");
    getline(cin,desc);
    dcmpi_string_trim(desc);

    std::string exe;
    printf("  Enter the executable you wish to call: ");
    getline(cin,exe);
    dcmpi_string_trim(exe);

    char * zErrMsg = NULL;
    std::string sql = "insert into registered_apps (name, executable, description) values (";
    sql += "'" + sql_escape(name) + "',";
    sql += "'" + sql_escape(exe) + "',";
    sql += "'" + sql_escape(desc) + "')";
    if (sqlite3_exec(db, sql.c_str(), NULL, NULL, &zErrMsg)) {
        std::cerr << "ERROR: " << zErrMsg
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }

    sql = "insert into app_IO (appname, varname, description, required, maxoccur, minoccur, IO) values ('";
    sql += sql_escape(name) + "','status','Status of execution.  \"OK\" for success, or an error message.','y',1,1,'O')";
    if (sqlite3_exec(db, sql.c_str(), NULL, NULL, &zErrMsg)) {
        std::cerr << "ERROR: " << zErrMsg
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    editapp(name);
}

void deleteapp()
{
    uint u;
    std::vector<DCApp> apps = getapps();
    if (apps.size()==0) {
        std::cerr << "ERROR: no apps registered\n";
        return;
    }
    std::vector<std::string> items;
    std::vector<std::string> out_choice;
    std::vector<int> out_choice_idx;
    for (u = 0; u < apps.size(); u++) {
        items.push_back(apps[u].name);
    }
    dcmpi_bash_select_emulator(
        2, items,
        "Please choose which app to delete:\n"
        "\n",
        "Exit",
        false, false,
        out_choice, out_choice_idx);
    if (out_choice.empty()) {
        return;
    }
    else {
        std::string app = out_choice[0];
        std::string sql = "delete from app_IO where appname = '";
        sql += sql_escape(app);
        sql += "'";
        char * zErrMsg = NULL;
        if (sqlite3_exec(db, sql.c_str(), NULL, NULL, &zErrMsg)) {
            std::cerr << "ERROR: " << zErrMsg
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
        sql = "delete from registered_apps where name = '";
        sql += sql_escape(app);
        sql += "'";
        if (sqlite3_exec(db, sql.c_str(), NULL, NULL, &zErrMsg)) {
            std::cerr << "ERROR: " << zErrMsg
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
    }
}

void appedit_main_menu()
{
    std::vector<std::string> items;
    std::vector<std::string> out_choice;
    std::vector<int> out_choice_idx;
    items.push_back("List applications");
    items.push_back("Register new application");
    items.push_back("Edit application");
    items.push_back("Delete application");
    
    while (1) {
        dcmpi_bash_select_emulator(
            0, items,
            "Please choose one of the following:\n"
            "\n",
            "Exit",
            false, false,
            out_choice, out_choice_idx);
        if (out_choice.empty()) {
            break;
        }
        else if (out_choice_idx[0] == 0) {
            listapps();
        }
        else if (out_choice_idx[0] == 1) {
            addapp();
        }
        else if (out_choice_idx[0] == 2) {
            editapp("");
        }
        else if (out_choice_idx[0] == 3) {
            deleteapp();
        }
    }
}

int main(int argc, char * argv[])
{
    appname = argv[0];
    bool systemwide = false;

    while (argc > 1) {
        if (!strcmp(argv[1], "-h")) {
            usage();
        }
        else if (!strcmp(argv[1], "-systemwide")) {
            systemwide = true;
        }
        else {
            break;
        }
        dcmpi_args_shift(argc, argv);
    }

    if ((argc-1) != 0) {
        usage();
    }

    dcmpi_configdb_init();
    if (systemwide) {
        db = db_systemwide_config;
    }
    else {
        db = db_user_config;
    }
    assert(db);
    printf("Welcome to dcmpi app registry\n\n");
    appedit_main_menu();
}
