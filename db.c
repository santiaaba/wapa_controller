#include "db.h"

void db_init(T_db *db){
	db->con = mysql_init(NULL);
}

int db_connect(T_db *db, T_config *c){
	if (!mysql_real_connect(db->con, c->db_server,
		c->db_user, c->db_pass, c->db_name, 0, NULL, 0)) {
		return 0;
	}
	return 1;
}

void db_close(T_db *db){
	mysql_close(db->con);
}

const char *db_error(T_db *db){
	return mysql_error(db->con);
}

void db_load_sites(T_db *db, T_list_site *l){
	char query[200];
	MYSQL_ROW row;
	T_site *new_site;

	strcpy(query,"select s.*, c.user_id from site s inner join suscription c on s.susc_id = c.id");

	mysql_query(db->con,query);
	MYSQL_RES *result = mysql_store_result(db->con);

	while ((row = mysql_fetch_row(result))){
		new_site = (T_site*)malloc(sizeof(T_site));
		printf("Nombre del sitio %s\n", row[2]);
		site_init(new_site,row[2],atoi(row[0]),atoi(row[5]),atoi(row[4]),atoi(row[1]),atoi(row[3]));
		printf("Agregamos sitio\n");
		list_site_add(l,new_site);
	}
	printf("Terminamos load sites\n");
}

void db_load_workers(T_db *db, T_list_worker *l){
	char query[200];
	MYSQL_ROW row;
	T_worker *new_worker;

	strcpy(query,"select * from worker");

	mysql_query(db->con,query);
	MYSQL_RES *result = mysql_store_result(db->con);

	while ((row = mysql_fetch_row(result))){
		new_worker = (T_worker*)malloc(sizeof(T_worker));
		worker_init(new_worker,atoi(row[0]),row[1],row[2],atoi(row[3]));
		list_worker_add(l,new_worker);
	}
	printf("Terminamos load workers\n");
}

void db_load_proxys(T_db *db, T_list_proxy *l){
	char query[200];
	MYSQL_ROW row;
	T_proxy *new_proxy;

	strcpy(query,"select * from site");

	mysql_query(db->con,query);
	MYSQL_RES *result = mysql_store_result(db->con);

	while ((row = mysql_fetch_row(result))){
		new_proxy = (T_proxy*)malloc(sizeof(T_proxy));
		proxy_init(new_proxy,row[1],row[2]);
		list_proxy_add(l,new_proxy);
	}
}
