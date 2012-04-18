#include "dcmpi_internal.h"

#include "dcmpi-configdb.h"

#include <dcmpi.h>
#include <dcmpi_util.h>
#include "dcmpisqlite3.h"

using namespace std;

void dcmpi_configdb_fini(void);

#if 0
static int callback(void *NotUsed, int argc, char **argv, char **azColName){
    int i;
    for(i=0; i<argc; i++){
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}
#endif

sqlite3 * db_systemwide_config = NULL;
sqlite3 * db_user_config = NULL;
void dcmpi_configdb_init(void)
{
    if (!db_systemwide_config && !db_user_config) {
        int rc = 0;
        std::vector<std::string> config_filenames = get_config_filenames("config.db", true);

        if (config_filenames[0] != "") {
            rc = sqlite3_open(config_filenames[0].c_str(), &db_systemwide_config);
            if (rc) {
                sqlite3_close(db_systemwide_config);
                db_systemwide_config = NULL;
            }
        }
        rc = sqlite3_open(config_filenames[1].c_str(), &db_user_config);
        if (rc) {
            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db_user_config));
            sqlite3_close(db_user_config);
            exit(1);
        }
        std::vector<const char*> idempotent;
        idempotent.push_back("create table if not exists registered_apps (name varchar(128), executable varchar(512), description varchar(4096))");
        idempotent.push_back("create table if not exists app_IO (appname varchar(128), varname varchar(128), description varchar(512), required varchar(1), minoccur int, maxoccur int, IO varchar(1))");
        char * zErrMsg = NULL;
        for (uint u = 0; u < idempotent.size(); u++) {
            if (db_systemwide_config) {
                if (sqlite3_exec(db_systemwide_config, idempotent[u], NULL, NULL, &zErrMsg)) {
                    std::cerr << "ERROR: " << zErrMsg
                              << " at " << __FILE__ << ":" << __LINE__
                              << std::endl << std::flush;
                    exit(1);
                }
            }
            if (db_user_config) {
                if (sqlite3_exec(db_user_config, idempotent[u], NULL, NULL, &zErrMsg)) {
                    std::cerr << "ERROR: " << zErrMsg
                              << " at " << __FILE__ << ":" << __LINE__
                              << std::endl << std::flush;
                    exit(1);
                }
            }
        }
        atexit(dcmpi_configdb_fini);
    }
}
void dcmpi_configdb_fini(void)
{
    if (db_systemwide_config) {
        sqlite3_close(db_systemwide_config);
        db_systemwide_config = NULL;
    }
    if (db_user_config) {
        sqlite3_close(db_user_config);
        db_user_config = NULL;
    }
}
