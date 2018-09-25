/* Posee los metodos y estructuras de datos para manejar el
 * acceso a la base de datos
 */

#include <mysql/mysql.h>
#include "config.h"
#include "structs.h"
#include <stdlib.h>

#ifndef DB_H
#define DB_H

typedef struct db {
	MYSQL *con;
} T_db;

void db_init(T_db *db);
int db_connect(T_db *db, T_config *c);
void db_close(T_db *db);
const char *db_error(T_db *db);
void db_load_sites(T_db *db, T_list_site *l);
void db_load_workers(T_db *db, T_list_worker *l);
int db_find_site(T_db *db, char *name);
void db_load_proxys(T_db *db, T_list_proxy *l);

/* Para los sitios */
int db_get_sites_id(T_db *db, char *susc_id, char **list_id, int *list_id_size);
int db_add_site(T_db *db, T_site **newsite, char *name, unsigned int susc_id, char *dir);
void db_site_list(T_db *db, char **data, int *data_size, char *susc_id);
int db_del_site(T_db *db, char *site_id);
int db_del_all_site(T_db *db, char *susc_id);
int db_get_hash_dir(T_db *db, char *site_id, char *hash_dir, char *site_name);

/* Para los workers */
void db_worker_start(T_db *db, int id);
void db_worker_stop(T_db *db, int id);

#endif
