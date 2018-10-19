#include "db.h"

/********************************
 * 	Funciones varias	*
 ********************************/

void random_dir(char *dir){
	/* Genera un dir y sub dir de dos digitos cada uno */
	char *string = "0123456789";
	int i,j;

	for(j=0;j<5;j++){
		if(j==2){
			dir[j]='/';
		} else {
			i = rand() % 10;
			dir[j] = string[i];
		}
	}
	dir[5]='\0';
}

int db_get_hash_dir(T_db *db, char *site_id, char *hash_dir,
		    char *site_name,char *error, int *db_fail,
		    T_logs *logs){

	char query[200];
	MYSQL_RES *result;
	MYSQL_ROW row;

	sprintf(query,"select name,hash_dir from web_site s inner join web_suscription u on (s.susc_id = u.id) where s.id=%s", site_id);
	logs_write(logs,L_DEBUG,"db_get_hash_dir",query);
	if(mysql_query(db->con,query)){
		*db_fail=1;
		logs_write(logs,L_ERROR,"db_get_hash_dir","DB_ERROR");
		return 0;
	}
	*db_fail = 0;
	result = mysql_store_result(db->con);
	if(mysql_num_rows(result) > 0){
		result = mysql_store_result(db->con);
		row = mysql_fetch_row(result);
		strcpy(hash_dir,row[1]);
		strcpy(site_name,row[0]);
		return 1;
	} else {
		sprintf(error,"300|\"code\":\"302\",\"info\":\"Imposible borrar. sitio no existe\"");
		return 0;
	}
}

/********************************
 * 	Funciones DB		*
 ********************************/

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

	sprintf(query,"select name from web_site where name='%s'",name);
	printf("Pasamos 1: %s\n",query);
	mysql_query(db->con,query);
	printf("Pasamos 2\n");
	MYSQL_RES *result = mysql_store_result(db->con);
	printf("Pasamos 3\n");
	if(mysql_num_rows(result) == 0){
		printf("Sitio no existe!!!\n");
		return 0;
	} else {
		printf("Sitio EXISTE!!!\n");
		return 1;
	}
}

uint16_t  db_site_exist(T_db *db, char *susc_id, char *site_id, char *error, int *db_fail, T_logs *logs){
	/* Si el sitio existe retorna su version. Si retorna 0 es porque
 	   no existe */
	char query[200];
	int resultado;
	MYSQL_RES *result;

	sprintf(query,"select id from web_site where id=%s and susc_id=%s", site_id,susc_id);
	mysql_query(db->con,query);
	result = mysql_store_result(db->con);
	if(!result){
        	logs_write(logs,L_ERROR,"db_site_exist", "DB_ERROR");
		*db_fail = 1;
		return 0;
	} else {
		*db_fail = 0;
		if(mysql_num_rows(result) > 0){
			return 1;
		} else {
			strcpy(error,"Sitio no existe");
			return 0;
		}
	}
}

/****	Funciones de carga	****/

int db_load_site_index(T_db *db, T_site *site, char *site_id, char *error, int *db_fail, T_logs *logs){
	/* Carga los indices de un sitio. Previamente vacia la estructura lista_s_e */
	char query[200];
	MYSQL_RES *result;
	MYSQL_ROW row;
	T_s_e *new_index;
	
	sprintf(query,"select id,name from web_indexes where site_id=%s\n",site_id);
        logs_write(logs,L_DEBUG,"db_load_sites_index", query);
	if(mysql_query(db->con,query)){
		/* Ocurrio un error */
        	logs_write(logs,L_ERROR,"db_load_sites_index", "DB_ERROR");
		*db_fail = 1;
	} else {
		db_fail = 0;
	}

	list_s_e_clean(site_get_indexes(site));
	result = mysql_store_result(db->con);
	while ((row = mysql_fetch_row(result))){
		new_index = (T_s_e*)malloc(sizeof(T_s_e));
		s_e_init(new_index,atoi(row[0]),row[1]);
		list_s_e_add(site_get_indexes(site),new_index);
	}
	return 1;
}

int db_load_site_alias(T_db *db, T_site *site, char *site_id, char *error, int *db_fail, T_logs *logs){
	/* Carga los alias de un sitio. Previamente vacia la estructura lista_s_e */
	char query[200];
	MYSQL_RES *result;
	MYSQL_ROW row;
	T_s_e *new_alias;
	
	sprintf(query,"select id,alias from web_alias where site_id=%s\n",site_id);
        logs_write(logs,L_DEBUG,"db_load_sites_alias",query);
	if(mysql_query(db->con,query)){
		/* Ocurrio un error */
        	logs_write(logs,L_ERROR,"db_load_sites_alias","DB_ERROR");
		*db_fail = 1;
		return 0;
	}

	db_fail = 0;
	list_s_e_clean(site_get_alias(site));
	result = mysql_store_result(db->con);
	while ((row = mysql_fetch_row(result))){
		new_alias = (T_s_e*)malloc(sizeof(T_s_e));
		s_e_init(new_alias,atoi(row[0]),row[1]);
		list_s_e_add(site_get_alias(site),new_alias);
	}
	return 1;
}

int db_load_sites(T_db *db, T_list_site *l, char *error, int *db_fail, T_logs *logs){
	char query[200];
	MYSQL_ROW row, row_alias;
	T_site *new_site;
	T_s_e *new_alias;

	strcpy(query,"select s.id,name,hash_dir,version,size from web_site s inner join web_suscription p on (s.susc_id = p.id)");
	logs_write(logs,L_DEBUG,"db_load_sites", query);

	if(mysql_query(db->con,query)){
		/* Ocurrio un error */
		logs_write(logs,L_ERROR,"db_load_sites","DB_ERROR");
		*db_fail = 1;
		return 0;
	}
	*db_fail = 0;
	MYSQL_RES *result = mysql_store_result(db->con);
	while ((row = mysql_fetch_row(result))){
		new_site = (T_site*)malloc(sizeof(T_site));
		site_init(new_site,row[1],atoi(row[0]),row[2],atoi(row[3]),atoi(row[4]));
		/* Cargamos los alias */
		if(!db_load_site_alias(db,new_site,row[0],error,db_fail,logs))
			return 0;
		/* Cargamos los indices */
		if(!db_load_site_index(db,new_site,row[0],error,db_fail,logs))
			return 0;
		/* Alta del sitio en la lista */
		list_site_add(l,new_site);
	}
	printf("Terminamos load sites\n");
	return 1;
}

int db_load_workers(T_db *db, T_list_worker *l, char *error, int *db_fail, T_logs *logs){
	char query[200];
	MYSQL_ROW row;
	T_worker *new_worker;

	strcpy(query,"select * from web_worker");
	logs_write(logs,L_DEBUG,"db_load_workers", query);

	if(mysql_query(db->con,query)){
		/* Ocurrio un error */
        	logs_write(logs,L_ERROR,"db_load_workers","DB_ERROR");
		*db_fail = 1;
		return 0;
	}
	*db_fail = 0;
	MYSQL_RES *result = mysql_store_result(db->con);
	while ((row = mysql_fetch_row(result))){
		new_worker = (T_worker*)malloc(sizeof(T_worker));
		worker_init(new_worker,atoi(row[0]),row[1],row[2],atoi(row[3]));
		list_worker_add(l,new_worker);
	}
	printf("Terminamos load workers\n");
	return 1;
}

int db_load_proxys(T_db *db, T_list_proxy *l, char *error, int *db_fail, T_logs *logs){
	char query[200];
	MYSQL_ROW row;
	T_proxy *new_proxy;

	strcpy(query,"select id,name,ipv4,status from web_proxy");
	logs_write(logs,L_DEBUG,"db_load_proxys", query);

	if(mysql_query(db->con,query)){
        	logs_write(logs,L_ERROR,"db_load_proxys","DB_ERROR");
		*db_fail = 1;
		return 0;
	}
	*db_fail = 0;
	MYSQL_RES *result = mysql_store_result(db->con);
	while ((row = mysql_fetch_row(result))){
		printf("Cargando Proxy %s\n",row[1]);
		new_proxy = (T_proxy*)malloc(sizeof(T_proxy));
		proxy_init(new_proxy,atoi(row[0]),row[1],row[2],atoi(row[3]));
		list_proxy_add(l,new_proxy);
	}
	return 1;
}

/****	Funciones para suscripciones	****/

int db_susc_add(T_db *db, char *susc_id, int *db_fail, T_logs *logs){
	char query[200];
	char hash_dir[6];

	random_dir(hash_dir);
	sprintf(query,"insert into web_suscription values (%s,'%s')",susc_id,hash_dir);
	printf("sql %s\n",query);
	logs_write(logs,L_DEBUG,"db_susc_add", query);
	if(mysql_query(db->con,query)){
        	logs_write(logs,L_ERROR,"db_susc_add","DB_ERROR");
		*db_fail = 1;
		return 0;
	}
	*db_fail = 0;
	return 1;
}

/****	Funciones para sitios	****/

int db_site_add(T_db *db, T_site **newsite, char *name, unsigned int susc_id,
		char *dir, char *error, int *db_fail, T_logs *logs){
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
	logs_write(logs,L_DEBUG,"db_site_add", query);

	if(mysql_query(db->con,query)){
		if(mysql_errno(db->con) == 1062){
			logs_write(logs,L_ERROR,"db_site_add", "DB_ERROR");
			*db_fail =0;
			logs_write(logs,L_ERROR,"db_site_add","Sitio con nombre repetido");
			strcpy(error,"300|\"code\":\"301\",\"info\":\"Ya existe sitio con ese nombre\"");
		} else {
			*db_fail =1;
			logs_write(logs,L_ERROR,"db_site_add","DB_ERROR");
		}
		return 0;
	} else {
		site_id = mysql_insert_id(db->con);

		/*Obtenemos el directorio */
		sprintf(query,"select hash_dir from web_suscription where id=%i",susc_id);
		printf("query: %s\n",query);
		if(mysql_query(db->con,query)){
			logs_write(logs,L_ERROR,"db_site_add", "DB_ERROR");
			*db_fail =1;
			return 0;
		}
		result = mysql_store_result(db->con);
		row = mysql_fetch_row(result);
		(*newsite) = (T_site *)malloc(sizeof(T_site));
		site_init(*newsite,name,site_id,row[0],1,1);
		strcpy(dir,row[0]);
		*db_fail =0;
		return 1;
	}
}

int db_site_mod(T_db *db, T_site *site, T_dictionary *d, char *error, int *db_fail, T_logs *logs){
	/* Modifica un sitio. Si no puede retorna 0. Si puede 1. Si falla
 	   la conexion a la base de datos retorna db_fail = 1. Sino 0. Si
	   retorna 0 y db_fail 0 entonces indica en error el motivo por el
	   cual no pudo realizar la modificacion */
	
	/* Todo chequeo previo sobre si es legal modificar el sitio
 	   debe haberse realizado previamente fuera de esta funcion */

	/* Si los datos no estan en *d es porque no se deben modificar */

	/* Es estado es un atributo que tiene su propia funcion (db_site_status) */

	char query[200];
	char *aux;
	int real_size;
	int pos;

	/* Modificamos configuracion del sitio */
	aux = dictionary_get(d,"version");
	if(!aux){
		return 0;
		strcpy(error,"FALTA VERSION");
	}
	strcpy(query,"update web_site set version= ");
	strcat(query, aux);
	aux = dictionary_get(d,"size");
	if(aux){
		strcat(query, " size=");
		strcat(query,aux);
	}
	strcat(query," where id= ");
	strcat(query,dictionary_get(d,"site_id"));
	printf("DB_SITE_MOD: %s\n",query);

	if(mysql_query(db->con,query)){
		/* Algo fallo */
        	logs_write(logs,L_ERROR,"db_site_mod","DB_ERROR");
		*db_fail = 1;
		return 0;
	}
	/* Cambios en la bse aplicados. Ahora se aplican al sitio */
	aux = dictionary_get(d,"size");
	if(aux)
		site_set_size(site,atoi(aux));
	aux = dictionary_get(d,"status");
	if(aux)
		site_set_size(site,atoi(aux));

	/* Modificamos los indices */
	if(dictionary_get(d,"index")){
		sprintf(query,"delete from web_indexes where d=%s",dictionary_get(d,"site_id"));
		printf("DB_SITE_MOD: %s\n",query);
		if(mysql_query(db->con,query)){
			/* Algo fallo */
        		logs_write(logs,L_ERROR,"db_site_mod","DB_ERROR");
			*db_fail = 1;
			return 0;
		}
		pos=0;
		strcpy(query,"insert into web_indexes(site_id,name) values ");
		real_size = strlen(dictionary_get(d,"index"));
		while(pos < real_size){
			parce_data(dictionary_get(d,"index"),',',&pos,aux);
			strcat(query,"(");
			strcat(query,dictionary_get(d,"site_id"));
			strcat(query,",");
			strcat(query,aux);
			strcat(query,"),");
		}
		query[strlen(query)-1] = '\0';
		printf("DB_SITE_MOD: %s\n",query);
		if(mysql_query(db->con,query)){
			/* Algo fallo */
        		logs_write(logs,L_ERROR,"db_site_mod","DB_ERROR");
			*db_fail = 1;
			return 0;
		}
	}
	/* Ahora modificamos los indices en el sitio */
	db_load_site_index(db,site,dictionary_get(d,"site_id"),error,db_fail,logs);
	if(db_fail) return 0;
	
	/* Modificamos los alias */
	if(dictionary_get(d,"alias")){
		sprintf(query,"delete from web_alias where d=%s",dictionary_get(d,"site_id"));
		printf("DB_SITE_MOD: %s\n",query);
		if(mysql_query(db->con,query)){
			/* Algo fallo */
        		logs_write(logs,L_ERROR,"db_site_mod","DB_ERROR");
			*db_fail = 1;
			return 0;
		}
		pos=0;
		strcpy(query,"insert into web_alias(site_id,alias) values ");
		real_size = strlen(dictionary_get(d,"alias"));
		while(pos < real_size){
			parce_data(dictionary_get(d,"alias"),',',&pos,aux);
			strcat(query,"(");
			strcat(query,dictionary_get(d,"site_id"));
			strcat(query,",");
			strcat(query,aux);
			strcat(query,")");
		}
		printf("DB_SITE_MOD: %s\n",query);
		if(mysql_query(db->con,query)){
			/* Algo fallo */
        		logs_write(logs,L_ERROR,"db_site_mod","DB_ERROR");
			*db_fail = 1;
			return 0;
		}
	}
	/* Ahora modificamos los alias en el sitio */
	db_load_site_alias(db,site,dictionary_get(d,"site_id"),error,db_fail,logs);
	if(db_fail) return 0;

	/* Le indicamos al sitio que se actualice en los workers */
	site_update(site);

	*db_fail = 0;
	return 1;
}

int db_site_status(T_db *db, char *susc_id, char *site_id, char *status, char *error, int *db_fail, T_logs *logs){
	/* Modifica el estado de un sitio */
	char query[200];

	if(!db_site_exist(db,susc_id,site_id,error,db_fail,logs))
		return 0;
	sprintf(query,"update web_site set status=%c where user_id=%s", status,site_id);
	if(mysql_query(db->con,query)){
		/* algo fallo */
        	logs_write(logs,L_ERROR,"db_site_mod","DB_ERROR");
		*db_fail = 1;
		return 0;
	}
	return 1;
}

int db_get_sites_id(T_db *db, char *susc_id, char **list_id, int *list_id_size){
	/* Retorna los sitios que posee una suscripcion dado el id
 	 * de la misma pasado por parametro */
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
	/* Retorna en formato json la lista de sitios dado el id de
 	 * una suscripcion pasado por parametro */

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
	strcpy(*data,"200|[");
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

void db_site_show(T_db *db, char **data, int *data_size, char *site_id, char *susc_id){
	/* Retorna en formato json los datos de un sitio dado
 	 * en base al id pasado por parametro */

	char query[200];
	char aux[500];
	MYSQL_RES *result;
	MYSQL_ROW row;

	sprintf(query,"select * from web_site where id =%s and susc_id=%s",site_id,susc_id);
	printf("DB_SITE_LIST: %s\n",query);
	mysql_query(db->con,query);
	if(result = mysql_store_result(db->con)){
		if(row = mysql_fetch_row(result)){
			*data_size = 500;
			printf("Allocamos memoria\n");
			*data=(char *)realloc(*data,*data_size);
			printf("colocamos info\n");
			sprintf(*data,"200|{\"id\":\"%s\",\"version\":\"%s\",\"name\":\"%s\",\"size\":\"%s\",\"susc_id\":\"%s\",\"status\":\"%s\",\"urls\":[",
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

int db_site_del(T_db *db, char *site_id, char *error, int *db_fail, T_logs *logs){
	/* Elimina un sitio y todas sus componentes
	   de la base de datos dado el id */

	char query[200];

	/* Borramos alias */
	sprintf(query,"delete from web_alias where site_id=%s",site_id);
	logs_write(logs,L_DEBUG,"db_site_del", query);
	if(mysql_query(db->con,query)){
		logs_write(logs,L_ERROR,"db_site_del","DB_ERROR");
		*db_fail=1;
		return 0;
	}

	/* Borramos indices */
	sprintf(query,"delete from web_indexes where site_id=%s",site_id);
	logs_write(logs,L_DEBUG,"db_site_del", query);
	if(mysql_query(db->con,query)){
		logs_write(logs,L_ERROR,"db_site_del","DB_ERROR");
		*db_fail=1;
		return 0;
	}

	/* Borrar entradas del ftp */
	/* IMPLEMENTAR */

	/* Borramos sitio */
	sprintf(query,"delete from web_site where id=%s",site_id);
	logs_write(logs,L_DEBUG,"db_site_del", query);
	if(mysql_query(db->con,query)){
		logs_write(logs,L_ERROR,"db_site_del","DB_ERROR");
		*db_fail=1;
		return 0;
	}
	return 1;
}

int db_del_all_site(T_db *db, char *susc_id, char *error, int *db_fail, T_logs *logs){
	/* Elimina todos los sitios de una suscripcion
 	 *  dado el id de la misma */
	char query[200];
	MYSQL_RES *result;
	MYSQL_ROW row;

	sprintf(query,"select id from web_site where susc_id=%s",susc_id);
	mysql_query(db->con,query);
	if(result = mysql_store_result(db->con)){
		while(row = mysql_fetch_row(result)){
			db_site_del(db,row[0],error,db_fail,logs);
		}
	} else {
		return 0;
	}
	return 1;
}

/****	Funciones para workers	****/

void db_worker_stop(T_db *db, int id){
	/* Deja constancia en la base de datos del worker
 	 * detenido dado el id del mismo */
	char query[200];
	sprintf(query,"update web_worker set status=1 where id=%i",id);
	printf("QEURY : %s\n",query);
	mysql_query(db->con,query);
}

void db_worker_start(T_db *db, int id){
	/* Deja constancia en la base de datos del worker
 	 * iniciado dado el id del mismo */
	char query[200];
	sprintf(query,"update web_worker set status=0 where id=%i",id);
	printf("QEURY : %s\n",query);
	mysql_query(db->con,query);
}
