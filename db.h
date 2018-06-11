/* Posee los metodos y estructuras de datos para manejar el
 * acceso a la base de datos
 */

#include "mysql.h"
#include "config.h"

typedef struct db {
	MYSQL *conn;
} T_db

void db_init(T_db *db);
int db_connect(T_db *db, T_config *c);
void db_close(t_db *db);

void db_load_sites(T_db *db, T_list_site *l);
void db_load_workers(T_db *db, T_list_worker *l);
void db_load_proxys(T_db *db, T_list_proxy *l);
