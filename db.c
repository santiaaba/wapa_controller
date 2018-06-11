#include "db.h"

void db_init(T_db *db){
	db->conn = mysql_init(NULL);
}
int db_connect(T_db *db T_config *c){
	if (!mysql_real_connect(db->conn, c->db_server,
		c->db_user, c->db_pass, c->db_name, 0, NULL, 0)) {
		return 0;
	}
	return 1;
}
void db_close(t_db *db){
}
void db_load_sites(T_db *db, T_list_site *l){
}
void db_load_workers(T_db *db, T_list_worker *l){
}
void db_load_proxys(T_db *db, T_list_proxy *l){
}
