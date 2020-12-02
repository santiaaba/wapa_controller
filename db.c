#include "db.h"

/********************************
 * 	Funciones varias	*
 ********************************/

void random_sitedir(char *name){
	/* *name debe ser un array de 20 elementos */
	char *string = "0123456789abcdefghijklmnopqrstuvwxyz";
	int j,i;

	for(j=0;j<19;j++){
		i = rand() % 36;
		name[j] = string[i];
	}
	name[19]='\0';
}

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

	sprintf(query,"select name,hash_dir from web_site s inner join web_namespace u on (s.namespace_id = u.id) where s.id=%s", site_id);
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

uint16_t  db_site_exist(T_db *db, char *namespace_id, char *site_id, char *error, int *db_fail){
	/* Si el sitio existe retorna su version. Si retorna 0 es porque
 	   no existe */
	char query[200];
	int resultado;
	MYSQL_RES *result;

	sprintf(query,"select id from web_site where id=%s and namespace_id=%s", site_id,namespace_id);
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
	char *new_index=NULL;
	MYSQL_RES *result;
	MYSQL_ROW row;
	
	sprintf(query,"select name from web_indexes where site_id=%s order by prioridad desc\n",site_id);
	printf("Query: %s\n",query);
	logs_write(db->logs,L_DEBUG,"db_load_sites_index", query);
	if(mysql_query(db->con,query)){
		/* Ocurrio un error */
		logs_write(db->logs,L_ERROR,"db_load_sites_index", "DB_ERROR");
		*db_fail = 1;
		return 0;
	} 

	db_fail = 0;
	lista_clean(site_get_indexes(site),free);
	result = mysql_store_result(db->con);
	while ((row = mysql_fetch_row(result))){
		dim_new(&new_index);
		dim_copy(&new_index,row[0]);
		printf("Agregando index: %s\n",new_index);
		lista_add(site_get_indexes(site),new_index);
	}
	return 1;
}

int db_load_site_alias(T_db *db, T_site *site, char *site_id, char *error, int *db_fail){
	/* Carga los alias de un sitio. */
	char query[200];
	MYSQL_RES *result;
	MYSQL_ROW row;
	char *new_alias;
	
	sprintf(query,"select alias from web_alias where site_id=%s\n",site_id);
	printf("Query: %s\n",query);
	logs_write(db->logs,L_DEBUG,"db_load_sites_alias",query);
	if(mysql_query(db->con,query)){
		/* Ocurrio un error */
		logs_write(db->logs,L_ERROR,"db_load_sites_alias","DB_ERROR");
		*db_fail = 1;
		return 0;
	}

	db_fail = 0;
	lista_clean(site_get_alias(site),free);
	result = mysql_store_result(db->con);
	while ((row = mysql_fetch_row(result))){
		dim_new(&new_alias);
		dim_copy(&new_alias,row[0]);
		printf("Agregando alias: %s\n",new_alias);
		lista_add(site_get_alias(site),new_alias);
	}
	printf("paso 3\n");
	if(lista_size(site_get_alias(site)) > 0){
		lista_first(site_get_alias(site));
		new_alias = lista_get(site_get_alias(site));
		printf("SIIIIIIIII:%s\n",new_alias);
	}
	return 1;
}

int db_load_sites(T_db *db, T_lista *l, char *error, int *db_fail){
	char query[200];
	char dir[200];
	char *ptr;
	MYSQL_ROW row, row_alias;
	T_site *new_site;
	T_s_e *new_alias;

	strcpy(query,"select s.id,s.name,n.hash_dir,s.version,s.size,s.dir, s.status,n.id, n.name from web_site s inner join web_namespace n on (s.namespace_id = n.id)");
	printf("Query: %s\n",query);
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
		sprintf(dir,"%s/%s",row[2],row[5]);
		site_init(new_site,row[1],strtoul(row[0],&ptr,10),dir,
					atoi(row[3]),atoi(row[4]),atoi(row[6]),atoi(row[7]),row[8]);
		printf("Llegamos\n");
		/* Cargamos los alias */
		if(!db_load_site_alias(db,new_site,row[0],error,db_fail))
			return 0;
		/* Cargamos los indices */
		if(!db_load_site_index(db,new_site,row[0],error,db_fail))
			return 0;
		/* Alta del sitio en la lista */
		lista_add(l,new_site);
	}
	printf("Terminamos load sites\n");
	return 1;
}

int db_load_workers(T_db *db, T_lista *l, char *error, int *db_fail){
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
	printf("Paso\n");
	while ((row = mysql_fetch_row(result))){
		new_worker = (T_worker*)malloc(sizeof(T_worker));
		worker_init(new_worker,atoi(row[0]),row[1],row[2],atoi(row[3]));
		lista_add(l,new_worker);
	}
	printf("Terminamos load workers\n");
	return 1;
}

int db_load_proxys(T_db *db, T_lista *l, char *error, int *db_fail){
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
		lista_add(l,new_proxy);
	}
	return 1;
}

/****	Funciones para namespace	****/
int db_namespaceName(T_db *db,char **message, uint32_t id, int *db_fail){
	char query[200];
	char aux[200];
	MYSQL_ROW row;
	MYSQL_RES *result;

	sprintf(query,"select name from web_namespace where id=%u",id);
	printf("SQL:%s\n",query);
	if(mysql_query(db->con,query)){
		printf("FALLO estamos\n");
		*db_fail = 1;
		return 0;
	}
	*db_fail = 0;
	printf("ACA estamos\n");
	result = mysql_store_result(db->con);
	row = mysql_fetch_row(result);
	dim_copy(message,row[0]);
	printf("Resultado:-%s-\n",*message);
	return 1;
}

int db_namespace_list(T_db *db, char **message, int *db_fail){
	char query[200];
	char aux[200];
	MYSQL_ROW row;
	MYSQL_RES *result;

	sprintf(query,"select * from web_namespace");
	printf("DB_NAMESPACE_LIST: %s\n",query);
	if(mysql_query(db->con,query)){
		printf("FALLO estamos\n");
		*db_fail = 1;
		return 0;
	}
	*db_fail = 0;
	printf("ACA estamos\n");
	result = mysql_store_result(db->con);
	json_mysql_result(result,message);
	printf("Resultado:-%s-\n",*message);
}

int db_namespace_show(T_db *db,char *namespace_id,char **message,int *db_fail){
	/* Retorna en formato json los datos de un namespace */
	char query[200];
	MYSQL_FIELD *field;
	int fields = 0;
	char *col_names[100];
	MYSQL_ROW row;
	MYSQL_RES *result;

	sprintf(query,"select * from web_namespace where id=%s",namespace_id);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_namespace_show","DB_ERROR");
		*db_fail = 1;
		return 0;
	}
	*db_fail = 0;
	result = mysql_store_result(db->con);

	while(field = mysql_fetch_field(result)){
		col_names[fields] = field->name;
		fields++;
	}

	if((mysql_num_rows(result) == 1)){
		row = mysql_fetch_row(result);
		json_mysql_result_row(row,col_names,fields,message);
		return 1;
	} else {
		dim_copy(message,"namespace no existe");
		return 0;
	}
}

int db_namespace_add(T_db *db, T_dictionary *d, int *db_fail){
	char query[200];
	char hash_dir[6];
	unsigned int namespace_id;

	printf("db_namespace_add\n");
	printf("-----------------\n");
	dictionary_print(d);
	printf("-----------------\n");
	/* Generamos la entrada en namespace */
	random_dir(hash_dir);
	sprintf(query,"insert into web_namespace(name,hash_dir) values ('%s','%s')",
			dictionary_get(d,"name"),hash_dir);

	printf("sql %s\n",query);
	logs_write(db->logs,L_DEBUG,"db_namespace_add", query);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_namespace_add","DB_ERROR");
		*db_fail = 1;
		return 0;
	}
	namespace_id = mysql_insert_id(db->con);

	/* Generamos la entrada para las tablas del servicio ftp */
	sprintf(query,"insert into ftpgroup(groupname,gid) values ('g_%u',%u)",namespace_id,namespace_id);
	printf("sql %s\n",query);
	logs_write(db->logs,L_DEBUG,"db_namespace_add", query);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_namespace_add","DB_ERROR");
		*db_fail = 1;
		return 0;
	}
	sprintf(query,"insert into ftpquotalimits(name,quota_type,limit_type,bytes_xfer_avail) values ('g_%u','group','hard',%u)",
		namespace_id,dictionary_get(d,"web_quota"));
	printf("sql %s\n",query);
	logs_write(db->logs,L_DEBUG,"db_namespace_add", query);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_namespace_add","DB_ERROR");
		*db_fail = 1;
		return 0;
	}
	sprintf(query,"insert into ftpquotatallies(name,quota_type) values ('g_%u','group')",namespace_id);
	printf("sql %s\n",query);
	logs_write(db->logs,L_DEBUG,"db_namespace_add", query);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_namespace_add","DB_ERROR");
		*db_fail = 1;
		return 0;
	}

	*db_fail = 0;
	return 1;
}

int db_namespace_del(T_db *db, char *namespace_id){
	/* Elimina un namespace. Previamente se deberia eliminar
	   los sitios de la misma. Si falla retorna 0 y se considera
	   una falla en la base de datos. Sino retorna 1 */
	char query[200];
	sprintf(query,"delete from web_namespace where id=%s",namespace_id);
	printf("query:%s\n",query);
	if(mysql_query(db->con,query)){
		return 0;
	}
	sprintf(query,"delete from ftpgroup where groupname='g_%s'",namespace_id);
	printf("query:%s\n",query);
	if(mysql_query(db->con,query)){
		return 0;
	}
	sprintf(query,"delete from ftpquotalimits where name='g_%s'",namespace_id);
	printf("query:%s\n",query);
	if(mysql_query(db->con,query)){
		return 0;
	}
	sprintf(query,"delete from ftpquotatallies where name='g_%s'",namespace_id);
	printf("query:%s\n",query);
	if(mysql_query(db->con,query)){
		return 0;
	}
	
	return 1;
}

/****	Funciones para sitios	****/

int db_limit_sites(T_db *db, char *namespace_id, int *db_fail){
	/* retorna 0 si se supera el limite de
	 * sitios del namespace o falla la base de datos*/
	char query[300];
	MYSQL_RES *result;
        MYSQL_ROW row;

	printf("Entro\n");
	sprintf(query,"select (select sites_limit from web_namespace where id=%s) - (select count(id) from web_site where namespace_id=%s)",namespace_id,namespace_id);
	printf(query);
	printf("PASO\n");
	if(mysql_query(db->con,query)){
		*db_fail = 1;
		printf("NO ALCANZO\n");
		return 0;
	}
	result = mysql_store_result(db->con);
	printf("NUMERO = %i\n",mysql_num_rows(result));
	row = mysql_fetch_row(result);
	/* Si el resultado esl NULL es porque hay una inconsistencia en la base de datos */
	if(!row[0]){
		*db_fail = 1;
		printf("NULL VALOR\n");
		return 0;
	}
	printf("ALCANZO\n");
	*db_fail = 0;
	return atoi(row[0]);
}

int db_site_add(T_db *db, T_site **newsite, char *name, uint32_t namespace_id,
				char *error, int *db_fail){
	/* Agrega un sitio a la base de datos.
 	 * Si no pudo hacerlo retorna 0 sino 1
	 * En el parametro dir retorna el directorio
	 * obtenido del namespace que le corresponde */
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
	uint32_t site_id;
	unsigned int index_id;
	T_s_e *newindex;
	int i;
	char hash_dir[20];	/* directorio del sitio */
	char newdir[30];	/* El directorio del namespace + directorio del sitio */
	char *namespaceName = NULL;

	if(!db_namespaceName(db,&namespaceName, namespace_id, db_fail)){
		return 0;
	}
	random_sitedir(hash_dir);

	sprintf(query,"insert into web_site(version,name,size,namespace_id,dir) values(1,\"%s\",1,%lu,\"%s\")",
		name,namespace_id,hash_dir);
	logs_write(db->logs,L_DEBUG,"db_site_add", query);
	printf("sql: %s\n", query);

	if(mysql_query(db->con,query)){
		printf("DB_SITE_ADD errno: %i\n",mysql_errno(db->con));
		if(mysql_errno(db->con) == 1062){
			*db_fail =0;
			logs_write(db->logs,L_ERROR,"db_site_add","Sitio con nombre repetido");
			strcpy(error,"Ya existe sitio con ese nombre");
		} else {
			*db_fail =1;
			logs_write(db->logs,L_ERROR,"db_site_add","DB_ERROR");
		}
		return 0;
	} else {
		site_id = mysql_insert_id(db->con);

		/*Obtenemos el directorio */
		sprintf(query,"select hash_dir from web_namespace where id=%lu",namespace_id);
		printf("query: %s\n",query);
		if(mysql_query(db->con,query)){
			logs_write(db->logs,L_ERROR,"db_site_add", "DB_ERROR");
			/* Va a quedar inconsistente porque pude agregar la entrada en la tabla del sitio
			 * pero a causa de este error no pude generar las otras en las demas tablas */
			*db_fail =1;
			return 0;
		}
		result = mysql_store_result(db->con);
		row = mysql_fetch_row(result);
		sprintf(newdir,"%s/%s",row[0],hash_dir);
		(*newsite) = (T_site *)malloc(sizeof(T_site));
		site_init(*newsite,name,site_id,newdir,1,1,S_ONLINE, namespace_id, namespaceName);

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
			lista_add(site_get_alias(*newsite),newindex);
		}
		*db_fail =0;
		return 1;
	}
}

int db_site_save(T_db *db, T_site *site, char *error, int *db_fail){
	/* Crea o modifica un sitio en la base de datos. Depende si ya existía.
  	   Si no puede retorna 0. Si puede 1. Si falla
 	   la conexion a la base de datos retorna db_fail = 1. Sino 0. Si
	   retorna 0 y db_fail 0 entonces indica en error el motivo por el
	   cual no pudo realizar la modificacion */
	
	/* Todo chequeo previo sobre si es legal modificar el sitio
 	   debe haberse realizado previamente fuera de esta funcion */

	/* Es estado es un atributo que tiene su propia funcion (db_site_status) */

	char *query=NULL;
	char aux[100];
	MYSQL_RES *result;
	int real_size;

	printf("Salvamos\n");
	/* Verificamos la existencia del sitio */
	
	sprintf(aux,"select id from web_site where id = %lu",site_get_id(site));
	dim_copy(&query,aux);
	printf("SQL: %s\n",query);
	if(mysql_query(db->con,query)){
		*db_fail = 1;
		return 0;
	}
	result = mysql_store_result(db->con);
	if((mysql_num_rows(result) == 1)){
		printf("Sitio existente\n");
		/* Ya existe el sitio y debe ser actualizado */
		sprintf(aux,"update web_site set version = %i, size= %i where id= %lu",
				site_get_version(site),
				site_get_size(site),
				site_get_id(site));
		dim_copy(&query,aux);
		printf("DB_SITE_MOD: %s\n",query);
	
		if(mysql_query(db->con,query)){
			/* Algo fallo */
			logs_write(db->logs,L_ERROR,"db_site_mod","DB_ERROR");
			*db_fail = 1;
			return 0;
		}
	} else {
		printf("Sitio nuevo\n");
		/* Es un sitio nuevo */
	}
	printf("PASOOOO!!!\n");
	/* Indices */
	sprintf(aux,"delete from web_indexes where site_id=%lu",site_get_id(site));
	dim_copy(&query,aux);
	printf("DB_SITE_MOD: %s\n",query);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_site_mod","DB_ERROR");
		*db_fail = 1;
		return 0;
	}
	if(lista_size(site_get_indexes(site)) > 0){
		dim_copy(&query,"insert into web_indexes(site_id,alias) values ");
		lista_first(site_get_indexes(site));
		while(!lista_eol(site_get_indexes(site))){
			dim_concat(&query,"(");
			sprintf(aux,"%lu",site_get_id(site));
			dim_concat(&query,aux);
			dim_concat(&query,",\'");
			dim_concat(&query,lista_get(site_get_indexes(site)));
			dim_concat(&query,"\'),");
			lista_next(site_get_indexes(site));
		}
		query[strlen(query)-1] = '\0';
		printf("DB_SITE_MOD: %s\n",query);
		if(mysql_query(db->con,query)){
			logs_write(db->logs,L_ERROR,"db_site_mod","DB_ERROR");
			*db_fail = 1;
			return 0;
		}
	}

	/* Alias */
	sprintf(aux,"delete from web_alias where site_id=%lu",site_get_id(site));
	dim_copy(&query,aux);
	printf("DB_SITE_MOD: %s\n",query);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_site_mod","DB_ERROR");
		*db_fail = 1;
		return 0;
	}
	if(lista_size(site_get_alias(site)) > 0){
		dim_copy(&query,"insert into web_alias(site_id,alias) values ");
		lista_first(site_get_alias(site));
		while(!lista_eol(site_get_alias(site))){
			dim_concat(&query,"(");
			sprintf(aux,"%lu",site_get_id(site));
			dim_concat(&query,aux);
			dim_concat(&query,",\'");
			dim_concat(&query,lista_get(site_get_alias(site)));
			dim_concat(&query,"\'),");
			lista_next(site_get_alias(site));
		}
		query[strlen(query)-1] = '\0';
		printf("DB_SITE_MOD: %s\n",query);
		if(mysql_query(db->con,query)){
			logs_write(db->logs,L_ERROR,"db_site_mod","DB_ERROR");
			*db_fail = 1;
			return 0;
		}
	}

	/* Le indicamos al sitio que se actualice en los workers */
	site_update(site);
	*db_fail = 0;
	return 1;
}

int db_site_status(T_db *db, char *namespace_id, char *site_id, char *status, char *error, int *db_fail){
	/* Modifica el estado de un sitio */
	char query[200];

	if(!db_site_exist(db,namespace_id,site_id,error,db_fail))
		return 0;
	sprintf(query,"update web_site set status=%s where id=%s and namespace_id=%s",
			status,site_id,namespace_id);
	printf("Query: %s\n",query);
	if(mysql_query(db->con,query)){
		/* algo fallo */
		logs_write(db->logs,L_ERROR,"db_site_mod","DB_ERROR");
		*db_fail = 1;
		return 0;
	}
	return 1;
}

int db_get_sites_id(T_db *db, char *namespace_id, int site_ids[256], int *site_ids_len, char *error, int *db_fail ){
	/* Retorna en la variable site_ids la cual es en array de int, el listado
 	 * de ids de sitios del namespace */
	char query[200];
	MYSQL_RES *result;
	MYSQL_ROW row;
	int i=0;
	
	sprintf(query,"select id from web_site where namespace_id=%s",namespace_id);
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

int db_site_list(T_db *db, T_lista *lista, char *namespace_id, int *db_fail){
	/* Retorna en el parametro list el listado de id de los sitios */

	char query[200];
	char *ptr;
	uint32_t *site_id;
	MYSQL_RES *result;
	MYSQL_ROW row;

	if(namespace_id[0] == 'A')
		sprintf(query,"select id from web_site");
	else
		sprintf(query,"select id from web_site where namespace_id =%s",namespace_id);
	if(mysql_query(db->con,query)){
		printf("FALLO estamos\n");
		*db_fail = 1;
		return 0;
	}
	*db_fail = 0;
	result = mysql_store_result(db->con);
	while(row = mysql_fetch_row(result)){
		site_id = (uint32_t *)malloc(sizeof(uint32_t));
		*site_id = strtoul(row[0],&ptr,10);
		printf("Guardamos id:%lu\n",*site_id);
		lista_add(lista,site_id);
		printf("pasamos\n");
	}
	printf("retornamos lista\n");
	return 1;
}

int db_site_show(T_db *db, char **message, char *site_id, char *namespace_id, int *db_fail){
	/* Retorna en formato json los datos de un sitio dado
 	 * en base al id pasado por parametro */

	char query[200];
	char *col_names[100];
	int fields = 0;
	MYSQL_FIELD *field;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int real_size;

	sprintf(query,"select * from web_site where id =%s and namespace_id=%s",site_id,namespace_id);
	printf("DB_SITE_LIST: %s\n",query);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_site_show","DB_ERROR");
		printf("Error contra la base de datos\n");
		*db_fail = 1;
		return 0;
	}
	*db_fail = 0;
	printf("paso\n");
	result = mysql_store_result(db->con);
	if((mysql_num_rows(result) == 1)){
		while(field = mysql_fetch_field(result)){
			col_names[fields] = field->name;
			fields++;
		}
		row = mysql_fetch_row(result);
		json_mysql_result_row(row,col_names,fields,message);
		return 1;
	} else {
		dim_copy(message,"sitio no existe");
		return 0;
	}
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
	/* Liberamos de la quota del namespace lo que ocupaba el sitio*/
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

int db_del_all_site(T_db *db, char *namespace_id, char *error, int *db_fail){
	/* Elimina todos los sitios de un namespace
 	 *  dado el id de la misma */
	char query[200];
	MYSQL_RES *result;
	MYSQL_ROW row;

	sprintf(query,"select id from web_site where namespace_id=%s",namespace_id);
	if(mysql_query(db->con,query)){
		*db_fail = 1;
		return 0;
	} else {
		result = mysql_store_result(db->con);
		while(row = mysql_fetch_row(result)){
			if(!db_site_del(db,row[0],0,error,db_fail))
				return 0;
		}

		/* Eliminamos la cuota utilizada del namespace */
		sprintf(query,"update ftpquotatallies set bytes_xfer_used=0 where name='g_%s'",
			namespace_id);
		if(mysql_query(db->con,query)){
			*db_fail = 1;
			return 0;
		}
	}
	return 1;
}

/****	Funciones para ftp	****/
int db_limit_ftp_users(T_db *db, char *site_id, int *db_fail){
	/* retorna 0 si se supera el limite de
	 * usuarios ftp del sitio o falla la base de datos*/
	char query[300];
	MYSQL_RES *result;
        MYSQL_ROW row;

	sprintf(query,"select (select s.ftp_per_site_limit from web_namespace s inner join web_site w on (s.id = w.namespace_id) where w.id=%s) - (select count(id) from ftpuser where site_id=%s)",site_id,site_id);
	if(mysql_query(db->con,query)){
		*db_fail = 1;
		return 0;
	}
	*db_fail = 0;
	result = mysql_store_result(db->con);
	row = mysql_fetch_row(result);
	return atoi(row[0]);
}

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
	MYSQL_ROW row_namespace;

	/*Obtenemos mediante el site_id el nombre del sitio y el namespace_id */
	printf("DB_ftp_add\n");
	*db_fail=0;
	sprintf(query,"select name, namespace_id from web_site where id=%s", dictionary_get(d,"site_id"));
	if(mysql_query(db->con,query)){
		printf("DB_SITE_ADD errno: %i\n",mysql_errno(db->con));
		*db_fail=1;
		return 0;
	}
	result = mysql_store_result(db->con);
	if(mysql_num_rows(result) < 1){
		printf("Sitio inexistente\n");
		strcpy(error,"300|\"code\":\"301\",\"info\":\"Sitio inexistente para crear usuario ftp\"");
		return 0;
	}
	row_site = mysql_fetch_row(result);

	/* Obtenemos el hash_dir */
	sprintf(query,"select hash_dir from web_namespace where id=%s",row_site[1]);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_ftp_add","DB_ERROR");
		*db_fail=1;
		return 0;
	}
	result = mysql_store_result(db->con);
	if(mysql_num_rows(result) < 1){
		strcpy(error,"300|\"code\":\"301\",\"info\":\"ERROR Fatal. Sitio uerfano\"");
		return 0;
	}
	row_namespace = mysql_fetch_row(result);
	
	/* Armamos el homedir */
	sprintf(homedir,"%s/%s/%s",config_webdir(c),row_namespace[0],row_site[0]);
	printf("homedir armado: %s\n",homedir);

	sprintf(query,"insert into ftpuser(userid,passwd,uid,gid,homedir,site_id) values('%s/%s','%s',%s,%s,'%s',%s)",
		dictionary_get(d,"name"),row_site[0],dictionary_get(d,"passwd"),config_ftpuid(c),
		row_site[1],homedir,dictionary_get(d,"site_id"));
	printf("QEURY : %s\n",query);
	if(mysql_query(db->con,query)){
		if(mysql_errno(db->con) == 1062){
			logs_write(db->logs,L_ERROR,"db_ftp_add","DB_ERROR: Usuario ftp repetido");
			strcpy(error,"300|\"code\":\"301\",\"info\":\"Ya existe un usuario ftp con ese nombre\"");
		} else {
			*db_fail=1;
			logs_write(db->logs,L_ERROR,"db_ftp_add","DB_ERROR");
		}
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

	*data=(char *)realloc(*data,real_size);
	result = mysql_store_result(db->con);
	strcpy(*data,"200|[");
	while(row = mysql_fetch_row(result)){
		exist = 1;
		sprintf(aux,"{\"id\":\"%s\",\"name\":\"%s\"},",row[0],row[1]);
		if(strlen(*data)+strlen(aux)+1 > real_size){
			real_size += 100;
			*data=(char *)realloc(*data,real_size);
		}
		strcat(*data,aux);
	}

	if(exist)
		(*data)[strlen(*data) - 1] = ']';
	else
		strcat(*data,"]");

	//Acomodamos para no desperdiciar memoria
	real_size = 1 + strlen(*data);
	*data=(char *)realloc(*data,real_size);
	return 1;
}

int db_ftp_del(T_db *db, T_dictionary *d, char *error, int *db_fail){
	/* Elimina una cuenta ftp */

	char query[200];
	/* Verificamos que el usuario ftp a eliminar corresponda al sitio indicado */
	sprintf(query,"delete from ftpuser where id=%s and site_id=%s",
		dictionary_get(d,"ftp_id"),
		dictionary_get(d,"site_id"));
	printf("QEURY : %s\n",query);
	if(mysql_query(db->con,query)){
		*db_fail = 1;
		return 0;
	}
	*db_fail=0;
	if(mysql_affected_rows(db->con)==0){
		strcpy(error,"300|\"code\":\"301\",\"info\":\"Usuario ftp inexistente para el sitio\"");
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

int db_login(T_db *db, T_dictionary *d, char *error, int *db_fail){
    MYSQL_ROW row;
    char sql[200];

    sprintf(sql,"select count(*) as cantidad from user where name=\"%s\" and pass=\"%s\"",
				dictionary_get(d,"user"),dictionary_get(d,"pass"));
    if(mysql_query(db->con,sql)){
                *db_fail = 1;
                return 0;
        }
    *db_fail = 0;
    MYSQL_RES *result = mysql_store_result(db->con);
    if((mysql_num_rows(result) == 1)){
        return 1;
    } else {
        sprintf(error,"{\"code\":\"350\",\"info\":\"Eror Login\"}");
        return 0;
    }
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
