/* Posee los metodos y estructuras de datos para manejar el
 * acceso a la base de datos
 */

#include <mysql/mysql.h>
#include <stdlib.h>
#include "config.h"
#include "structs.h"
#include "dictionary.h"
#include "logs.h"

#ifndef DB_H
#define DB_H

typedef enum {DB_ONLINE, DB_OFFLINE} T_DB_status;

typedef struct db {
	MYSQL *con;
	T_logs *logs;
	char user[100];
	char pass[100];
	char server[100];
	char dbname[100];
	T_DB_status status;
} T_db;

void db_init(T_db *db, T_config *c, T_logs *logs);
int db_connect(T_db *db);
void db_close(T_db *db);
int db_live(T_db *db);

const char *db_error(T_db *db);
int db_load_sites(T_db *db, T_list_site *l, char *error, int *db_fail);
int db_load_site_index(T_db *db, T_site *site, char *site_id, char *error, int *db_fail);
int db_load_site_alias(T_db *db, T_site *site, char *site_id, char *error, int *db_fail);
int db_load_workers(T_db *db, T_list_worker *l, char *error, int *db_fail);
int db_load_proxys(T_db *db, T_list_proxy *l, char *error, int *db_fail);
int db_find_site(T_db *db, char *name);

/* Para los sitios */
int db_get_sites_id(T_db *db, char *susc_id, int site_ids[256], int *site_ids_len, char *error, int *db_fail );
int db_site_add(T_db *db, T_site **newsite, char *name,
		unsigned int susc_id, char *dir, char *error,
		int *db_fail);

int db_site_mod(T_db *db, T_site *site, T_dictionary *d, char *error, int *db_fail);
void db_site_list(T_db *db, char **data, char *susc_id);
int db_site_del(T_db *db, char *site_id, uint32_t size, char *error, int *db_fail);
uint16_t db_site_exist(T_db *db, char *susc_id, char *site_id, char *error, int *db_fail);
void db_site_show(T_db *db, char **data, char *site_id, char *susc_id);
int db_site_status(T_db *db, char *susc_id, char *site_id, char *status, char *error, int *db_fail);
int db_del_all_site(T_db *db, char *susc_id, char *error, int *db_fail);
int db_get_hash_dir(T_db *db, char *site_id, char *hash_dir, char *site_name,char *error, int *db_fail);

/* Para los usuarios ftp */
int db_get_ftp_id(T_db *db, char *site_id, int ftp_ids[256], int *ftp_ids_len, char *error, int *db_fail );
int db_ftp_list(T_db *db, char **data, char *site_id);
int db_ftp_add(T_db *db, T_dictionary *d, T_config *c, char *error, int *db_fail);
int db_ftp_del(T_db *db, char *ftp_id);

/* Para las suscripcionse */
int db_susc_show(T_db *db,char *susc_id,char **message,int *db_fail);
int db_susc_add(T_db *db, T_dictionary *d, int *db_fail);
int db_susc_del(T_db *db, char *susc_id);


/* Para los servers en general */
void db_server_start(T_db *db, int id);
void db_server_stop(T_db *db, int id);
int db_worker_get_info(T_db *db, int id, char *data);
int db_proxy_get_info(T_db *db, int id, char *data);
#endif
