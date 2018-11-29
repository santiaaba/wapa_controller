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
		    char *site_name,char *error, int *db_fail){

	char query[200];
	MYSQL_RES *result;
	MYSQL_ROW row;

	sprintf(query,"select name,hash_dir from web_site s inner join web_suscription u on (s.susc_id = u.id) where s.id=%s", site_id);
	printf("%s\n",query);
	logs_write(db->logs,L_DEBUG,"db_get_hash_dir",query);
	if(mysql_query(db->con,query)){
		*db_fail=1;
		logs_write(db->logs,L_ERROR,"db_get_hash_dir","DB_ERROR");
		return 0;
	}
	*db_fail = 0;
	result = mysql_store_result(db->con);
	printf("Estamos aca\n");
	if(mysql_num_rows(result) > 0){
		row = mysql_fetch_row(result);
		printf("Llegamos Estamos aca\n");
		strcpy(site_name,row[0]);
		strcpy(hash_dir,row[1]);
		printf("Llegamos Estamos aca\n");
		return 1;
	} else {
		sprintf(error,"300|\"code\":\"302\",\"info\":\"Imposible borrar. sitio no existe\"");
		return 0;
	}
}

/********************************
 * 	Funciones DB		*
 ********************************/

void db_init(T_db *db, T_config *c, T_logs *logs){
	db->status = DB_ONLINE;
	db->logs = logs;
	strcpy(db->user,c->db_user);
	strcpy(db->pass,c->db_pass);
	strcpy(db->server,c->db_server);
	strcpy(db->dbname,c->db_name);
}

int db_connect(T_db *db){
	db->con = mysql_init(NULL);
	printf("Conectando DB: %s,%s,%s,%s\n",db->server,db->user, db->pass, db->dbname);
	if (mysql_real_connect(db->con, db->server,
		db->user, db->pass, db->dbname, 0, NULL, 0)){
		db->status = DB_ONLINE;
		printf("Conecto\n");
		return 1;
	} else {
		printf("NO Conecto\n");
		db_close(db);
		return 0;
	}
}

int db_live(T_db *db){

	MYSQL_RES *result;

	if(mysql_query(db->con,"select * from version")){
		db_close(db);
		return 0;
	} else {
		result = mysql_store_result(db->con);
		return 1;
	}
}

void db_close(T_db *db){
	printf("Cerramos base de datos\n");
	mysql_close(db->con);
	db->status = DB_OFFLINE;
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

uint16_t  db_site_exist(T_db *db, char *susc_id, char *site_id, char *error, int *db_fail){
	/* Si el sitio existe retorna su version. Si retorna 0 es porque
 	   no existe */
	char query[200];
	int resultado;
	MYSQL_RES *result;

	sprintf(query,"select id from web_site where id=%s and susc_id=%s", site_id,susc_id);
	mysql_query(db->con,query);
	result = mysql_store_result(db->con);
	if(!result){
		logs_write(db->logs,L_ERROR,"db_site_exist", "DB_ERROR");
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

int db_load_site_index(T_db *db, T_site *site, char *site_id, char *error, int *db_fail){
	/* Carga los indices de un sitio. Previamente vacia la estructura lista_s_e */
	char query[200];
	MYSQL_RES *result;
	MYSQL_ROW row;
	T_s_e *new_index;
	
	sprintf(query,"select id,name from web_indexes where site_id=%s order by prioridad desc\n",site_id);
	logs_write(db->logs,L_DEBUG,"db_load_sites_index", query);
	if(mysql_query(db->con,query)){
		/* Ocurrio un error */
		logs_write(db->logs,L_ERROR,"db_load_sites_index", "DB_ERROR");
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

int db_load_site_alias(T_db *db, T_site *site, char *site_id, char *error, int *db_fail){
	/* Carga los alias de un sitio. Previamente vacia la estructura lista_s_e */
	char query[200];
	MYSQL_RES *result;
	MYSQL_ROW row;
	T_s_e *new_alias;
	
	sprintf(query,"select id,alias from web_alias where site_id=%s\n",site_id);
	logs_write(db->logs,L_DEBUG,"db_load_sites_alias",query);
	if(mysql_query(db->con,query)){
		/* Ocurrio un error */
		logs_write(db->logs,L_ERROR,"db_load_sites_alias","DB_ERROR");
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

int db_load_sites(T_db *db, T_list_site *l, char *error, int *db_fail){
	char query[200];
	MYSQL_ROW row, row_alias;
	T_site *new_site;
	T_s_e *new_alias;

	strcpy(query,"select s.id,name,hash_dir,version,size from web_site s inner join web_suscription p on (s.susc_id = p.id)");
	logs_write(db->logs,L_DEBUG,"db_load_sites", query);

	if(mysql_query(db->con,query)){
		/* Ocurrio un error */
		logs_write(db->logs,L_ERROR,"db_load_sites","DB_ERROR");
		*db_fail = 1;
		return 0;
	}
	*db_fail = 0;
	MYSQL_RES *result = mysql_store_result(db->con);
	while ((row = mysql_fetch_row(result))){
		new_site = (T_site*)malloc(sizeof(T_site));
		site_init(new_site,row[1],atoi(row[0]),row[2],atoi(row[3]),atoi(row[4]));
		/* Cargamos los alias */
		if(!db_load_site_alias(db,new_site,row[0],error,db_fail))
			return 0;
		/* Cargamos los indices */
		if(!db_load_site_index(db,new_site,row[0],error,db_fail))
			return 0;
		/* Alta del sitio en la lista */
		list_site_add(l,new_site);
	}
	printf("Terminamos load sites\n");
	return 1;
}

int db_load_workers(T_db *db, T_list_worker *l, char *error, int *db_fail){
	char query[200];
	MYSQL_ROW row;
	T_worker *new_worker;

	/* No es necesario filtrar por el rol en un where porque el inner join ya lo hace */
	strcpy(query,"select s.*, w.size from web_server s inner join web_worker w on (s.id = w.id)");
	logs_write(db->logs,L_DEBUG,"db_load_workers", query);

	if(mysql_query(db->con,query)){
		/* Ocurrio un error */
		logs_write(db->logs,L_ERROR,"db_load_workers","DB_ERROR");
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

int db_load_proxys(T_db *db, T_list_proxy *l, char *error, int *db_fail){
	char query[200];
	MYSQL_ROW row;
	T_proxy *new_proxy;

	strcpy(query,"select s.* from web_server s where s.rol=1");
	logs_write(db->logs,L_DEBUG,"db_load_proxys", query);

	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_load_proxys","DB_ERROR");
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

int db_susc_show(T_db *db,char *susc_id,char **message,int *db_fail){
	/* Retorna en formato json los datos de una suscripcion */
	char query[200];
	char aux[200];
	MYSQL_ROW row;
	MYSQL_RES *result;

	sprintf(query,"select * from web_suscription where id=%s",susc_id);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_susc_show","DB_ERROR");
		*db_fail = 1;
		return 0;
	}
	*db_fail = 0;
	result = mysql_store_result(db->con);
	strcpy(aux,"200|\"cloud\":\"Hosting web\",\"data\":\"Nada de momento\"}");
	printf("DB_SUSC_SHOW: %s\n",aux);
	*message=(char *)realloc(*message,strlen(aux)+1);
	strcpy(*message,aux);
	return 1;
	
}

int db_susc_add(T_db *db, T_dictionary *d, int *db_fail){
	char query[200];
	char hash_dir[6];
	char *susc_id;

	/* Generamos la entrada en web_suscription */
	random_dir(hash_dir);
	susc_id = dictionary_get(d,"susc_id");
	sprintf(query,"insert into web_suscription values (%s,'%s')",susc_id,hash_dir);
	printf("sql %s\n",query);
	logs_write(db->logs,L_DEBUG,"db_susc_add", query);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_susc_add","DB_ERROR");
		*db_fail = 1;
		return 0;
	}

	/* Generamos la entrada para las tablas del servicio ftp */
	sprintf(query,"insert into ftpgroup(groupname,gid) values (g_%s,'%s')",susc_id,susc_id);
	printf("sql %s\n",query);
	logs_write(db->logs,L_DEBUG,"db_susc_add", query);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_susc_add","DB_ERROR");
		*db_fail = 1;
		return 0;
	}
	sprintf(query,"insert into ftpquotalimits(name,quota_type,limit_type,bytes_xfer_avail) values (g_%s,'group','hard',%s)",
		susc_id,dictionary_get(d,"size"));
	printf("sql %s\n",query);
	logs_write(db->logs,L_DEBUG,"db_susc_add", query);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_susc_add","DB_ERROR");
		*db_fail = 1;
		return 0;
	}
	sprintf(query,"insert into ftpquotatallies(name,quota_type) values (g_%s,'group')",susc_id);
	printf("sql %s\n",query);
	logs_write(db->logs,L_DEBUG,"db_susc_add", query);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_susc_add","DB_ERROR");
		*db_fail = 1;
		return 0;
	}


	*db_fail = 0;
	return 1;
}

int db_susc_del(T_db *db, char *susc_id){
	/* Elimina una suscripcion. Previamente se deberia eliminar
	   los sitios de la misma. Si falla retorna 0 y se considera
	   una falla en la base de datos. Sino retorna 1 */
	char query[200];
	sprintf(query,"delete from web_suscription where id=%s",susc_id);
	printf("query:%s\n",query);
	if(mysql_query(db->con,query)){
		return 0;
	}
	sprintf(query,"delete from ftpgroup where name='g_%s'",susc_id);
	printf("query:%s\n",query);
	if(mysql_query(db->con,query)){
		return 0;
	}
	sprintf(query,"delete from ftpquotalimits where name='g_%s'",susc_id);
	printf("query:%s\n",query);
	if(mysql_query(db->con,query)){
		return 0;
	}
	sprintf(query,"delete from ftpquotatallies where name='g_%s'",susc_id);
	printf("query:%s\n",query);
	if(mysql_query(db->con,query)){
		return 0;
	}
	
	return 1;
}

/****	Funciones para sitios	****/

int db_site_add(T_db *db, T_site **newsite, char *name, unsigned int susc_id,
		char *dir, char *error, int *db_fail){
	/* Agrega un sitio a la base de datos.
 	 * Si no pudo hacerlo retorna 0 sino 1
	 * En el parametro dir retorna el directorio
	 * obtenido de la suscripcion que le corresponde */
	char index[6][15] = {
				"default.html",
				"index.htm",
				"index.html",
				"index.asp",
				"index.aspx",
				"index.php"
	};
	char query[300];
	MYSQL_RES *result;
	MYSQL_ROW row;
	unsigned int site_id;
	unsigned int index_id;
	T_s_e *newindex;
	int i;

	sprintf(query,"insert into web_site(version,name,size,susc_id) values(1,\"%s\",1,%lu)",
		name,susc_id,dir);
	logs_write(db->logs,L_DEBUG,"db_site_add", query);

	if(mysql_query(db->con,query)){
		printf("DB_SITE_ADD errno: %i\n",mysql_errno(db->con));
		if(mysql_errno(db->con) == 1062){
			logs_write(db->logs,L_ERROR,"db_site_add", "DB_ERROR");
			*db_fail =0;
			logs_write(db->logs,L_ERROR,"db_site_add","Sitio con nombre repetido");
			strcpy(error,"300|\"code\":\"301\",\"info\":\"Ya existe sitio con ese nombre\"");
		} else {
			*db_fail =1;
			logs_write(db->logs,L_ERROR,"db_site_add","DB_ERROR");
		}
		printf("Pasamos\n");
		return 0;
	} else {
		site_id = mysql_insert_id(db->con);

		/*Obtenemos el directorio */
		sprintf(query,"select hash_dir from web_suscription where id=%i",susc_id);
		printf("query: %s\n",query);
		if(mysql_query(db->con,query)){
			logs_write(db->logs,L_ERROR,"db_site_add", "DB_ERROR");
			*db_fail =1;
			return 0;
		}
		result = mysql_store_result(db->con);
		row = mysql_fetch_row(result);
		(*newsite) = (T_site *)malloc(sizeof(T_site));
		site_init(*newsite,name,site_id,row[0],1,1);
		strcpy(dir,row[0]);

		/* Poblamos los indices */
		for(i=5;i>=0;i--){
			sprintf(query,"insert into web_indexes(site_id,name,prioridad) values(%i,'%s',%i)",
				site_id,index[i],i);
			if(mysql_query(db->con,query)){
				logs_write(db->logs,L_ERROR,"db_site_add", "DB_ERROR");
				*db_fail =1;
				return 0;
			}
			index_id = mysql_insert_id(db->con);
			newindex = (T_s_e *)malloc(sizeof(T_s_e));
			s_e_init(newindex,index_id,index[i]);
			list_s_e_add(site_get_alias(*newsite),newindex);
		}
		*db_fail =0;
		return 1;
	}
}

int db_site_mod(T_db *db, T_site *site, T_dictionary *d, char *error, int *db_fail){
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
		logs_write(db->logs,L_ERROR,"db_site_mod","DB_ERROR");
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
			logs_write(db->logs,L_ERROR,"db_site_mod","DB_ERROR");
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
			logs_write(db->logs,L_ERROR,"db_site_mod","DB_ERROR");
			*db_fail = 1;
			return 0;
		}
	}
	/* Ahora modificamos los indices en el sitio */
	db_load_site_index(db,site,dictionary_get(d,"site_id"),error,db_fail);
	if(db_fail) return 0;
	
	/* Modificamos los alias */
	if(dictionary_get(d,"alias")){
		sprintf(query,"delete from web_alias where d=%s",dictionary_get(d,"site_id"));
		printf("DB_SITE_MOD: %s\n",query);
		if(mysql_query(db->con,query)){
			/* Algo fallo */
			logs_write(db->logs,L_ERROR,"db_site_mod","DB_ERROR");
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
			logs_write(db->logs,L_ERROR,"db_site_mod","DB_ERROR");
			*db_fail = 1;
			return 0;
		}
	}
	/* Ahora modificamos los alias en el sitio */
	db_load_site_alias(db,site,dictionary_get(d,"site_id"),error,db_fail);
	if(db_fail) return 0;

	/* Le indicamos al sitio que se actualice en los workers */
	site_update(site);

	*db_fail = 0;
	return 1;
}

int db_site_status(T_db *db, char *susc_id, char *site_id, char *status, char *error, int *db_fail){
	/* Modifica el estado de un sitio */
	char query[200];

	if(!db_site_exist(db,susc_id,site_id,error,db_fail))
		return 0;
	sprintf(query,"update web_site set status=%c where user_id=%s", status,site_id);
	if(mysql_query(db->con,query)){
		/* algo fallo */
		logs_write(db->logs,L_ERROR,"db_site_mod","DB_ERROR");
		*db_fail = 1;
		return 0;
	}
	return 1;
}

int db_get_sites_id(T_db *db, char *susc_id, int site_ids[256], int *site_ids_len, char *error, int *db_fail ){
	/* Retorna en la variable site_ids la cual es en array de int, el listado
 	 * de ids de sitios de la suscripcion */
	char query[200];
	MYSQL_RES *result;
	MYSQL_ROW row;
	int i=0;
	
	sprintf(query,"select id from web_site where susc_id=%s",susc_id);
	printf("QEURY : %s\n",query);
	if(mysql_query(db->con,query)){
		*db_fail = 1;
		return 0;
	}
	*db_fail = 0;
	printf("paso\n");
	result = mysql_store_result(db->con);
	while(row = mysql_fetch_row(result)){
		site_ids[i] = atoi(row[0]);
		i++;
	}
	*site_ids_len = i;
	printf("Termino\n");
	return 1;
}

void db_site_list(T_db *db, char **data, char *susc_id){
	/* Retorna en formato json la lista de sitios dado el id de
 	 * una suscripcion pasado por parametro */

	const int max_c_site=300; //Los datos de un solo sitio no deben superar este valor
	char query[200];
	char aux[max_c_site];
	int real_size;
	int exist=0;
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
		exist = 1;
		printf("Agregando\n");
		sprintf(aux,"{\"id\":\"%s\",\"name\":\"%s\",\"status\":\"%s\"},",row[0],row[1],row[2]);
		if(strlen(*data)+strlen(aux)+1 < real_size){
			real_size =+ max_c_site;
			*data=(char *)realloc(*data,real_size);
		}
		strcat(*data,aux);
	}
	if(exist){
		(*data)[strlen(*data) - 1] = ']';
	} else {
		strcat(*data,"]");
	}
	// Redimencionamos para no desperdiciar memoria
	*data=(char *)realloc(*data,strlen(*data)+1);
	printf("Resultado:-%s-\n",*data);
}

void db_site_show(T_db *db, char **data, char *site_id, char *susc_id){
	/* Retorna en formato json los datos de un sitio dado
 	 * en base al id pasado por parametro */

	char query[200];
	char aux[500];
	MYSQL_RES *result;
	MYSQL_ROW row;
	int real_size;

	sprintf(query,"select * from web_site where id =%s and susc_id=%s",site_id,susc_id);
	printf("DB_SITE_LIST: %s\n",query);
	mysql_query(db->con,query);
	if(result = mysql_store_result(db->con)){
		if(row = mysql_fetch_row(result)){
			printf("Allocamos memoria\n");
			real_size = 500;
			*data=(char *)realloc(*data,real_size);
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
				if(strlen(*data)+strlen(aux)+1 < real_size){
					real_size =+ 200;
					*data=(char *)realloc(*data,real_size);
				}
				strcat(*data,aux);
			}
			// Reemplazamos la última "," por "]"
			(*data)[strlen(*data) - 1] = ']';
			//Cerramos los datos con '}' y redimencionamos el string
		} else {
			real_size = 100;
			*data=(char *)realloc(*data,real_size);
			sprintf(*data,"{\"error\":\"site not exist\"");
		}
	} else {
		real_size = 100;
		*data=(char *)realloc(*data,real_size);
		sprintf(*data,"{\"error\":\"db error\"");
	}
	real_size = strlen(*data) + 2;
	*data=(char *)realloc(*data,real_size);
	strcat(*data,"}");
}

int db_site_del(T_db *db, char *site_id, uint32_t size, char *error, int *db_fail){
	/* Elimina un sitio y todas sus componentes
	   de la base de datos dado el id */

	char query[200];

	/* Borrar entradas del ftp */
	sprintf(query,"delete from ftpuser where site_id=%s",site_id);
	logs_write(db->logs,L_DEBUG,"db_site_del", query);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_site_del","DB_ERROR");
		*db_fail=1;
		return 0;
	}
	/* Liberamos de la quota de la suscripcion lo que ocupaba el sitio*/
	sprintf(query,"update ftpquotatallies set bytes_xfer_used = bytes_xfer_used - %i",size);
	logs_write(db->logs,L_DEBUG,"db_site_del", query);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_site_del","DB_ERROR");
		*db_fail=1;
		return 0;
	}

	/* Borramos alias */
	sprintf(query,"delete from web_alias where site_id=%s",site_id);
	logs_write(db->logs,L_DEBUG,"db_site_del", query);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_site_del","DB_ERROR");
		*db_fail=1;
		return 0;
	}

	/* Borramos indices */
	sprintf(query,"delete from web_indexes where site_id=%s",site_id);
	logs_write(db->logs,L_DEBUG,"db_site_del", query);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_site_del","DB_ERROR");
		*db_fail=1;
		return 0;
	}

	/* Borramos sitio */
	sprintf(query,"delete from web_site where id=%s",site_id);
	logs_write(db->logs,L_DEBUG,"db_site_del", query);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_site_del","DB_ERROR");
		*db_fail=1;
		return 0;
	}
	return 1;
}

int db_del_all_site(T_db *db, char *susc_id, char *error, int *db_fail){
	/* Elimina todos los sitios de una suscripcion
 	 *  dado el id de la misma */
	char query[200];
	MYSQL_RES *result;
	MYSQL_ROW row;

	sprintf(query,"select id from web_site where susc_id=%s",susc_id);
	if(mysql_query(db->con,query)){
		*db_fail = 1;
		return 0;
	} else {
		result = mysql_store_result(db->con);
		while(row = mysql_fetch_row(result)){
			if(!db_site_del(db,row[0],0,error,db_fail))
				return 0;
		}

		/* Eliminamos la cuota utilizada de la suscripcion */
		sprintf(query,"update ftpquotatallies set bytes_xfer_used=0 where name='g_%s'",
			susc_id);
		if(mysql_query(db->con,query)){
			*db_fail = 1;
			return 0;
		}
	}
	return 1;
}

/****	Funciones para ftp	****/

int db_ftp_add(T_db *db, T_dictionary *d, T_config *c, char *error, int *db_fail){
	/* Agrega un usuario ftp. Los siguientes datos deben venir en el diccionario
 	site_id		id del sitio
	user_id		string
	passwd		string
	*/

	char query[200];
	char homedir[200];
	MYSQL_RES *result;
	MYSQL_ROW row_site;
	MYSQL_ROW row_susc;

	/*Obtenemos mediante el site_id el nombre del sitio y el susc_id */
	*db_fail=0;
	sprintf(query,"select name, susc_id from web_site where id=%s",
		dictionary_get(d,"site_id"));
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_ftp_add","DB_ERROR");
		*db_fail=1;
		return 0;
	}
	if(result = mysql_store_result(db->con)){
		row_site = mysql_fetch_row(result);
	} else {
		printf("VEREMOS que tiramos de error\n");
		return 0;
	}

	/* Obtenemos el hash_dir */
	sprintf(query,"select hash_dir from web_suscription where id=%s",row_site[1]);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_ftp_add","DB_ERROR");
		*db_fail=1;
		return 0;
	}
	if(result = mysql_store_result(db->con)){
		row_susc = mysql_fetch_row(result);
	} else {
		printf("VEREMOS que tiramos de error\n");
		return 0;
	}
	
	/* Armamos el homedir */
	sprintf(homedir,"%s/%s/%s",config_webdir(c),row_susc[0],row_site[0]);

	sprintf(query,"insert into ftpuser(userid,passwd,uid,gid,homedir,site_id) values('%s/%s','%s',%s,%s,'%s',%s)",
		dictionary_get(d,"user_id"),row_site[0],dictionary_get(d,"passwd"),config_ftpuid(c),
		row_site[1],homedir,dictionary_get(d,"site_id"));
	printf("QEURY : %s\n",query);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_ftp_user_list","DB_ERROR");
		*db_fail=1;
		return 0;
	}
	return 1;
}

int db_get_ftp_id(T_db *db, char *site_id, int ftp_ids[256], int *ftp_ids_len, char *error, int *db_fail ){
	/* Retorna el listado de ids de usuarios ftp de un sitio dado */
	char query[200];
	MYSQL_RES *result;
	MYSQL_ROW row;
	int i=0;

	sprintf(query,"select id from ftpuser where site_id=%s",site_id);
	printf("QEURY : %s\n",query);
	if(mysql_query(db->con,query)){
		*db_fail = 1;
		return 0;
	}
	*db_fail = 0;
	result = mysql_store_result(db->con);
	while(row = mysql_fetch_row(result)){
		ftp_ids[i] = atoi(row[0]);
		i++;
	}
	*ftp_ids_len = i;
	return 1;
}

int db_ftp_list(T_db *db, char **data, char *site_id){
	/* Retorna en **data en formato json los usuarios ftp de un sitio dado mediante el
 	 * parametro site_id. Si existen problemas con la base de datos retorna 0. Sino 1. */

	char query[200];
	char aux[200];
	MYSQL_RES *result;
        MYSQL_ROW row;
	int exist;
	int real_size = 200;

	sprintf(query,"select id, userid from ftpuser where site_id=%s",site_id);
	printf("QEURY : %s\n",query);
	if(mysql_query(db->con,query))
		return 0;

	*data=(char *)realloc(*data,real_size * sizeof(char));
	result = mysql_store_result(db->con);
	strcpy(*data,"200|[");
	while(row = mysql_fetch_row(result)){
		exist = 1;
		sprintf(aux,"{\"id\":\"%s\",\"name\":\"%s\"},",row[0],row[1]);
		if(strlen(*data)+strlen(aux)+1 < real_size){
			real_size =+ 100;
			*data=(char *)realloc(*data,real_size);
		}
		strcat(*data,aux);
	}
	if(exist){
		(*data)[strlen(*data) - 1] = ']';
	} else {
		strcat(*data,"]");
	}
	//Acomodamos para no desperdiciar memoria
	*data=(char *)realloc(*data,strlen(*data)+1);
	printf("Resultado:-%s-\n",*data);
	return 1;
}

int db_ftp_del(T_db *db, char *ftp_id){
	/* Elimina una cuenta ftp */

	char query[200];
	sprintf(query,"delete from ftpuser where id=%s",ftp_id);
	printf("QEURY : %s\n",query);
	if(mysql_query(db->con,query)){
		return 0;
	}
	return 1;
}

/****	Funciones para servers	****/

void db_server_stop(T_db *db, int id){
	/* Deja constancia en la base de datos del server
 	 * detenido dado el id del mismo */
	char query[200];
	sprintf(query,"update web_server set status=1 where id=%i",id);
	printf("QEURY : %s\n",query);
	mysql_query(db->con,query);
}

void db_server_start(T_db *db, int id){
	/* Deja constancia en la base de datos del server
 	 * iniciado dado el id del mismo */
	char query[200];
	sprintf(query,"update web_server set status=0 where id=%i",id);
	printf("QEURY : %s\n",query);
	mysql_query(db->con,query);
}

int db_worker_get_info(T_db *db, int id, char *data){
	/* Retorna en formato json en *data informacion guardada
	 * del worker. Retorna 0 si falla la conexion a la base */
	char query[200];
	char status[10];
	MYSQL_RES *result;
	MYSQL_ROW row;

	sprintf(query,"select s.status, w.size from web_server s inner join web_worker w on (s.id = w.id) where s.id = %i",id);
	printf("query: %s\n",query);
	if(mysql_query(db->con,query)){
		return 0;
	}
	if(result = mysql_store_result(db->con)){
		row = mysql_fetch_row(result);
		itowstatus(atoi(row[0]),status);
		sprintf(data,"\"status\":\"%s\",\"size\":%s,",status,row[1]);
	}
	return 1;
}

int db_proxy_get_info(T_db *db, int id, char *data){
	/* Retorna en formato json en *data informacion guardada
	 * del worker. Retorna 0 si falla la conexion a la base */
	char query[200];
	char status[10];
	MYSQL_RES *result;
	MYSQL_ROW row;

	sprintf(query,"select s.status from web_server s where id = %i",id);
	printf("Query: %s\n",query);
	if(mysql_query(db->con,query))
		return 0;
	if(result = mysql_store_result(db->con)){
		row = mysql_fetch_row(result);
		itowstatus(atoi(row[0]),status);
		sprintf(data,"\"status\":%s,",status);
	}
	return 1;
}
