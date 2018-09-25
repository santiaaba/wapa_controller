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

int db_find_site(T_db *db, char *name){
	/* Determina si un sitio ya existe en la base de datos */

	char query[200];
	int resultado;
	MYSQL_ROW row;

	sprintf(query,"select count(*) from web_site where name='%s'",name);
	printf("Pasamos 1: %s\n",query);
	mysql_query(db->con,query);
	printf("Pasamos 2\n");
	MYSQL_RES *result = mysql_store_result(db->con);
	printf("Pasamos 3\n");
	row = mysql_fetch_row(result);
	if(atoi(row[0]) == 0){
		printf("Sitio no existe!!!\n");
		return 0;
	} else {
		printf("Sitio EXISTE!!!\n");
		return 1;
	}
}

void db_load_sites(T_db *db, T_list_site *l){
	char query[200];
	MYSQL_ROW row, row_alias;
	T_site *new_site;
	T_s_e *new_alias;

	strcpy(query,"select s.id,name,hash_dir,version,size from web_site s inner join web_suscription p on (s.susc_id = p.id)");

	mysql_query(db->con,query);
	MYSQL_RES *result = mysql_store_result(db->con);

	while ((row = mysql_fetch_row(result))){
		new_site = (T_site*)malloc(sizeof(T_site));
		
		site_init(new_site,row[1],atoi(row[0]),row[2],atoi(row[3]),atoi(row[4]));

		/* Cargamos los alias */
		sprintf(query,"select id,alias from web_alias where site_id=%s\n",row[0]);
		printf("Consulta para obtener los alias del sitio id %s: %s\n",row[0],query);
		mysql_query(db->con,query);
		MYSQL_RES *result_alias = mysql_store_result(db->con);
		while ((row_alias = mysql_fetch_row(result_alias))){
			printf("Agregando alias %s\n",row_alias[1]);
			new_alias = (T_s_e*)malloc(sizeof(T_s_e));
			s_e_init(new_alias,atoi(row_alias[0]),row_alias[1]);
			list_s_e_add(site_get_alias(new_site),new_alias);
		}
		list_site_add(l,new_site);
	}
	printf("Terminamos load sites\n");
}

void db_load_workers(T_db *db, T_list_worker *l){
	char query[200];
	MYSQL_ROW row;
	T_worker *new_worker;

	strcpy(query,"select * from web_worker");

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

	strcpy(query,"select id,name,ipv4,status from web_proxy");

	mysql_query(db->con,query);
	MYSQL_RES *result = mysql_store_result(db->con);

	while ((row = mysql_fetch_row(result))){
		printf("Cargando Proxy %s\n",row[1]);
		new_proxy = (T_proxy*)malloc(sizeof(T_proxy));
		proxy_init(new_proxy,atoi(row[0]),row[1],row[2],atoi(row[3]));
		list_proxy_add(l,new_proxy);
	}
}

int db_add_site(T_db *db, T_site **newsite, char *name, unsigned int susc_id, char *dir){
	/* Agrega un sitio a la base de datos.
 	 * Si no pudo hacerlo retorna 0 sino 1
	 * En el parametro dir retorna el directorio
	 * obtenido de la suscripcion que le corresponde */

	char query[300];
	MYSQL_RES *result;
	MYSQL_ROW row;
	unsigned int site_id;

	sprintf(query,"insert into web_site(version,name,size,susc_id) values(1,\"%s\",1,%lu)",
                name,susc_id,dir);

	mysql_query(db->con,query);
	if ((result = mysql_store_result(db->con)) == 0 &&
		mysql_field_count(db->con) == 0 &&
		mysql_insert_id(db->con) != 0){
		
		site_id = mysql_insert_id(db->con);

		/*Obtenemos el directorio */
		sprintf(query,"select hash_dir from web_suscription where id=%i",susc_id);
		printf("query: %s\n",query);
		mysql_query(db->con,query);
		result = mysql_store_result(db->con);
		if(row = mysql_fetch_row(result)){
			(*newsite) = (T_site *)malloc(sizeof(T_site));
			site_init(*newsite,name,site_id,row[0],1,1);
			strcpy(dir,row[0]);
		} else {
			return 0;
		}
	} else {
		return 0;
	}
	return 1;
}

void db_worker_stop(T_db *db, int id){
	char query[200];

	sprintf(query,"update web_worker set status=1 where id=%i",id);
	printf("QEURY : %s\n",query);
	mysql_query(db->con,query);
}

void db_worker_start(T_db *db, int id){
	char query[200];

	sprintf(query,"update web_worker set status=0 where id=%i",id);
	printf("QEURY : %s\n",query);
	mysql_query(db->con,query);
}

int db_get_sites_id(T_db *db, char *susc_id, char **list_id, int *list_id_size){
	char query[200];
        MYSQL_RES *result;
        MYSQL_ROW row;
	
	sprintf(query,"select id from web_site where susc_id=%c",susc_id);
	printf("QEURY : %s\n",query);
        mysql_query(db->con,query);

	*list_id_size=40;
	*list_id = (char *)realloc(*list_id,*list_id_size);
	while(row = mysql_fetch_row(result)){
		if(*list_id_size < strlen(row[0] + strlen(*list_id)) + 2){
			*list_id_size+=40;	// Se que mas de 40 no ocupa un site_id + "," + "\0";
			*list_id = (char *)realloc(*list_id,*list_id_size);
		}
		strcat(*list_id,"%s");
		strcat(*list_id,",");
	}
	*list_id[(*list_id_size)-1] = '\0';
	return 1;
}

void db_site_list(T_db *db, char **data, int *data_size, char *susc_id){
	/* Lo retornamos en formato json */
	const int max_c_site=300; //Los datos de un solo sitio no deben superar este valor
	
	char query[200];
	char aux[max_c_site];
	int real_size;
	MYSQL_RES *result;
        MYSQL_ROW row;

        sprintf(query,"select id,name,status from web_site where susc_id =%s",susc_id);
	printf("DB_SITE_LIST: %s\n",query);
	mysql_query(db->con,query);
	result = mysql_store_result(db->con);

	*data=(char *)realloc(*data,max_c_site);
	real_size = max_c_site;
	strcpy(*data,"[");
	while(row = mysql_fetch_row(result)){
		printf("Agregando\n");
		sprintf(aux,"{\"id\":\"%s\",\"name\":\"%s\",\"status\":\"%s\"},",row[0],row[1],row[2]);
		if(strlen(*data)+strlen(aux)+1 < real_size){
			real_size =+ max_c_site;
			*data=(char *)realloc(*data,real_size);
		}
		strcat(*data,aux);
	}
	(*data)[strlen(*data) - 1] = ']';
	printf("Resultado:-%s-\n",*data);
	*data_size = real_size;
}

void db_site_show(T_db *db, char **data, int *data_size, char *site_id){

	char query[200];
	char aux[500];
	MYSQL_RES *result;
	MYSQL_ROW row;

	sprintf(query,"select * from web_site where id =%s",site_id);
	printf("DB_SITE_LIST: %s\n",query);
	mysql_query(db->con,query);
	if(result = mysql_store_result(db->con)){
		if(row = mysql_fetch_row(result)){
			*data_size = 500;
			printf("Allocamos memoria\n");
			*data=(char *)realloc(*data,*data_size);
			printf("colocamos info\n");
			sprintf(*data,"{\"id\":\"%s\",\"version\":\"%s\",\"name\":\"%s\",\"size\":\"%s\",\"susc_id\":\"%s\",\"status\":\"%s\",\"urls\":[",
				row[0],row[1],row[2],row[3],row[4],row[5]);
	
			/* Listado de alias */
			sprintf(query,"select id,alias from web_alias where site_id =%s",site_id);
			printf("DB_SITE_LIST: %s\n",query);
			mysql_query(db->con,query);
			result = mysql_store_result(db->con);
			while(row = mysql_fetch_row(result)){
				sprintf(aux,"{\"id\":\"%s\",\"url\":\"%s\"),",row[0],row[1]);
				//Si no entra en *data, reallocamos para que entre y un poco mas
				if(strlen(*data)+strlen(aux)+1 < *data_size){
					*data_size =+ 200;
					*data=(char *)realloc(*data,*data_size);
				}
				strcat(*data,aux);
			}
			// Reemplazamos la última "," por "]"
			(*data)[strlen(*data) - 1] = ']';
			//Cerramos los datos con '}' y redimencionamos el string
		} else {
			*data_size = 100;
			*data=(char *)realloc(*data,*data_size);
			sprintf(*data,"{\"error\":\"site not exist\"");
		}
	} else {
		*data_size = 100;
		*data=(char *)realloc(*data,*data_size);
		sprintf(*data,"{\"error\":\"db error\"");
	}
	*data_size = strlen(*data) + 2;
	*data=(char *)realloc(*data,*data_size);
	strcat(*data,"}");
}

int db_del_site(T_db *db, char *site_id){
	char query[200];

	/* Borramos alias */
	sprintf(query,"delete from web_alias where site_id=%s",site_id);
	printf("db_del_site: %s\n",query);
	mysql_query(db->con,query);

	/* Borramos indices */
	sprintf(query,"delete from web_indexes where site_id=%s",site_id);
	printf("db_del_site: %s\n",query);
	mysql_query(db->con,query);

	/* Borrar entradas del ftp */
	/* IMPLEMENTAR */

	/* Borramos sitio */
	sprintf(query,"delete from web_site where id=%s",site_id);
	printf("db_del_site: %s\n",query);
	mysql_query(db->con,query);
	return 1;
}

int db_del_all_site(T_db *db, char *susc_id){
	char query[200];
	MYSQL_RES *result;
        MYSQL_ROW row;

	sprintf(query,"select id from web_site where susc_id=%s",susc_id);
        mysql_query(db->con,query);
	if(result = mysql_store_result(db->con)){
		while(row = mysql_fetch_row(result)){
			db_del_site(db,row[0]);
		}
	} else {
		return 0;
	}
	return 1;
}

int db_get_hash_dir(T_db *db, char *site_id, char *hash_dir, char *site_name){
	char query[200];
	MYSQL_RES *result;
        MYSQL_ROW row;

	sprintf(query,"select name,hash_dir from web_site s inner join web_suscription u on (s.susc_id = u.id) where s.id=%s", site_id);
        mysql_query(db->con,query);
	if(result = mysql_store_result(db->con)){
		if(row = mysql_fetch_row(result)){
			strcpy(hash_dir,row[1]);
			strcpy(site_name,row[0]);
			return 1;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}
