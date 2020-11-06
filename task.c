#include "task.h"

void random_task_id(T_taskid value){
	/*Genera un string random para task_id */
	char *string = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	int i,j;
	
	//for(j=0;j<TASKID_SIZE-1;j++){
	for(j=0;j<TASKID_SIZE;j++){
		i = rand() % 62;
		value[j] = string[i];
	}
	value[TASKID_SIZE] = '\0';
}

void random_token(T_tasktoken value){
	/*Genera un string random para token_id */
	char *string = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	int i,j;

	for(j=0;j<TOKEN_SIZE;j++){
		i = rand() % 62;
		value[j] = string[i];
	}
	value[TOKEN_SIZE] = '\0';
}

int valid_task_id(char *s){
	int ok=1;
	int size;
	int i=0;

	if(s == NULL)
		return 0;
	if(strlen(s) != TASKID_SIZE)
		return 0;
	while(ok && i<TASKID_SIZE){
		ok = ((47 < (int)s[i] && (int)s[i] < 58) ||     //numeros
		     (64 < (int)s[i] && (int)s[i] < 91) ||      //letras mayusculas
		     (96 < (int)s[i] && (int)s[i] < 123));       //letras minusculas
		i++;
	}
	return ok;
}

void task_print_status(T_task *t, char *s){
    switch(t->status){
    case T_WAITING: strcpy(s,"waiting"); break;
    case T_RUNNING: strcpy(s,"running"); break;
    case T_TODO: strcpy(s,"todo"); break;
    case T_DONE: strcpy(s,"done"); break;
    }
}

void task_json_result(T_task *t, char **result){
    /* Aloca memoria y concatena la respuesta del task que ya deberia venir
	   en formato json agregando dicho contenido en un strig que se entregara
	   a los clientes tambien en forma json */

    char aux[20];

    task_print_status(t,aux);
    if(t->status == T_DONE){
		/* Task finalizado */
        *result=(char *)realloc(*result,strlen(t->result) + 200);
        sprintf(*result,"{\"taskid\":\"%s\",\"status\":\"%s\",\"data\":%s}",t->id,aux,t->result);
    } else {
		/* Task no finalizado */
        *result=(char *)realloc(*result,200);
        sprintf(*result,"{\"taskid\":\"%s\",\"status\":\"%s\"}",t->id,aux);
    }
	printf("task_json_result:%s\n",*result);
}

T_task_type task_c_to_type(char c){
	/* Hay dos restricciones.
 	 * 't': Es utilizado cuando el core solicita el estado de un task
 	 * 'c': Es utilizado cuando el core solicita un chequeo al controller
 	 */
	switch(c){
		case '0': return T_NAMESPACE_ADD;
		case '1': return T_NAMESPACE_DEL;
		case '2': return T_NAMESPACE_STOP;
		case '3': return T_NAMESPACE_START;
		case '4': return T_NAMESPACE_SHOW;

		case 'l': return T_SITE_LIST;
		case 'q': return T_SITE_LIST_ALL;
		case 's': return T_SITE_SHOW;
		case 'a': return T_SITE_ADD;
		case 'm': return T_SITE_MOD;
		case 'd': return T_SITE_DEL;
		case 'k': return T_SITE_STOP;
		case 'e': return T_SITE_START;

		case 'b': return T_FTP_LIST;
		case 'h': return T_FTP_ADD;
		case 'f': return T_FTP_DEL;
		case 'g': return T_FTP_MOD;

		case 'L': return T_SERVER_LIST;
		case 'S': return T_SERVER_SHOW;
		case 'A': return T_SERVER_ADD;
		case 'M': return T_SERVER_MOD;
		case 'D': return T_SERVER_DEL;
		case 'K': return T_SERVER_STOP;
		case 'E': return T_SERVER_START;
	}
}

/****************************************
	     Verificadiones
*****************************************/

/* Antes por cada tarea verificabamos que el diccionario en data
 * tuviese todos los datos necesarios. Esa responsabilidad se la
 * pasamos a server.c server.h. Los mismos no generan un task
 * si no estan todos los datos necesarios
*/

/*****************************
	     TASK 
******************************/
void task_init(T_task *t){
	random_task_id(t->id);
	t->type = T_TASK_NONE;
	t->time = time(NULL);
	t->result = NULL;
	dim_copy(&(t->result),"");
	t->data = NULL;
	t->status = T_TODO;
}

void task_set(T_task *t, T_task_type type, T_dictionary *data){
	printf("Task_set asignando tipo: %i\n",type);
	t->type = type;
	t->data = data;
}

void task_destroy(T_task **t){
	printf("Destruir task\n");
	if((*t)->data != NULL){
		printf("Task: liberando diccionario\n");
		dictionary_destroy(&((*t)->data));
		printf("Task: liberando diccionario fin\n");
	}
	if((*t)->result != NULL){
		printf("Task ahora el resultado\n");
		printf("Task: liberando resultado: %p\n",(*t)->result);
		free((*t)->result);
		printf("Task: liberando resultado fin\n");
	}
	free(*t);
}

time_t task_get_time(T_task *t){
	return t->time;
}

char *task_get_token(T_task *t){
	return t->token;
}

char *task_get_result(T_task *t){
	return t->result;
}

char *task_get_id(T_task *t){
	return t->id;
}

void task_done(T_task *t, int http_code, char *message){
	printf("Task DONE: '%s'\n",message);
	t->status = T_DONE;
	printf("Task DONE 1?\n");
	t->http_code = http_code;
	printf("Task DONE 2?\n");
	t->time = time(NULL);
	printf("Task DONE 3?\n");
	dim_copy(&(t->result),message);
	printf("Task DONE 4?\n");
	//strcpy(t->result,message);
	printf("Task DONE\n");
}

T_dictionary *task_get_data(T_task *t){
	return t->data;
}

int task_login(T_task *t, T_db *db){
    /* Autentica un usuario y le retorna un token */
    int db_fail;
    char message[200];
    char token[100] = "fijodemomento";

    if(!db_login(db,t->data,message,&db_fail)){
        if(db_fail)
            task_done(t,HTTP_501,M_DB_ERROR);
        else
            task_done(t,HTTP_401,M_LOGIN_ERROR);
    }
    /* Generamos un token de forma random */
    sprintf(message,"{\"code\":\"200\",\"token\":\"%s\"}",token);
    task_done(t,HTTP_200,message);
}

void task_site_list(T_task *t, T_db *db, T_logs *logs){
	/* Lista sitios de un namespace dado */
	char *namespace_id;
	char *message=NULL;
	int db_fail;

	printf("Santiagooooo task_site_list\n");
	namespace_id = dictionary_get(t->data,"namespace_id");
	db_site_list(db,&message,namespace_id,&db_fail);
	if(db_fail)
        task_done(t,HTTP_501,M_DB_ERROR);
    else
        task_done(t,HTTP_200,message);
}

T_task_status task_get_status(T_task *t){
    return t->status;
}

void task_site_show(T_task *t, T_db *db, T_logs *logs){
	char *site_id;
	char *namespace_id;
	T_site *site;

	site_id = dictionary_get(t->data,"site_id");
	namespace_id = dictionary_get(t->data,"namespace_id");
	db_site_show(db,&(t->result),site_id,namespace_id);
}

int task_site_add(T_task *t, T_lista *l, T_db *db, T_config *config, T_logs *logs){
	/* Agrega un sitio a la solucion */

	T_site *newsite;
	char error[200];
	int db_fail;
	char sql[300];
	char *name;
	char *namespace_id;
	char command[300];
	char dir[100];
	unsigned int id;
	
	dictionary_print(t->data);
	name = dictionary_get(t->data,"name");
	namespace_id = dictionary_get(t->data,"namespace_id");

	/* Verificamos que no se supere el limite de sitios establecidos
	   para el namespace y obtenemos su hashdir */
	printf("Verificamos limite sitios\n");
	if(0 == db_limit_sites(db, namespace_id, &db_fail)){
		if(db_fail)
			task_done(t,HTTP_501,M_DB_ERROR);
		else
			task_done(t,HTTP_410,M_SITE_LIMT_ERROR);
		return 0;
	}
	
	// Alta en la base de datos del sitio
	printf("Alta en base de datos\n");
	if(!db_site_add(db,&newsite,name,atoi(namespace_id),error,&db_fail)){
		if(db_fail){
			task_done(t,HTTP_501,M_DB_ERROR);
		} else {
			task_done(t,HTTP_411,error);
		}
		printf("Paso\n");
		return 0;
	}

	// Creacion del espacio de almacenamiento
	sprintf(command,"mkdir -p /%s/%s/wwwroot",config_webdir(config),site_get_dir(newsite));
	SYSTEM_DO

	sprintf(command,"mkdir -p /%s/%s/logs",config_webdir(config),site_get_dir(newsite));
	SYSTEM_DO

	sprintf(command,"chown -R ftpuser:root /%s/%s/",config_webdir(config),site_get_dir(newsite));
	SYSTEM_DO

	sprintf(command,"chmod -R 755 /%s/%s/",config_webdir(config),site_get_dir(newsite));
	SYSTEM_DO

	sprintf(command,"chmod -R 555 /%s/%s/logs",config_webdir(config),site_get_dir(newsite));
	SYSTEM_DO

	sprintf(command,"cp -pr %s/* /%s/%s/wwwroot/",config_default(config),config_webdir(config),
	site_get_dir(newsite));
	SYSTEM_DO

	// Adicion del sitio a la lista
	printf("agregado al listado\n");
	lista_add(l,newsite);

	task_done(t,HTTP_200,"Sitio agregado");
	return 1;
}

int task_site_del(T_task *t, T_lista *l, T_db *db, T_config *c, T_logs *logs){
	/* Borra fisicamente y logicamente un sitio. Borramos ademas las cuentas ftp
 	 * si pudo borrarlo retorna 1. Si no pudo o * fue borrado parcialmente retorna 0 */
	char hash_dir[10];
	char error[200];
	char site_name[100];
	char command[200];
	char *site_id;
	int db_fail;
	uint32_t size;
	char ftp_id[50];
	int ftp_ids[256];	//Maxima cantidad de ftp users por sitio
	int ftp_ids_len;	//cantoidad de usuarios ftp
	int ok;
	T_site *site = NULL;
	T_worker *worker;
	T_task *task_aux=NULL;
	T_dictionary *data_aux=NULL;

	site_id = dictionary_get(t->data,"site_id");

	/* Eliminamos las cuentas FTP */
	task_aux = (T_task *)malloc(sizeof(T_task));
	data_aux = (T_dictionary *)malloc(sizeof(T_dictionary));
	dictionary_init(data_aux);
	task_init(task_aux);
	task_set(task_aux,T_FTP_DEL,data_aux);
	if(!db_get_ftp_id(db,site_id,ftp_ids,&ftp_ids_len,error,&db_fail)){
		if(db_fail)
			task_done(t,HTTP_501,M_DB_ERROR);
		else
			task_done(t,HTTP_499,error);
	} else {
		ok = 1;
		dictionary_add(task_aux->data,"site_id",site_id);
		while((ftp_ids_len > 0) && ok){
			sprintf(ftp_id,"%i",ftp_ids[ftp_ids_len - 1 ]);
			dictionary_add(task_aux->data,"ftp_id",ftp_id);
			dictionary_print(task_aux->data);
			ok &= task_ftp_del(task_aux,db);
			dictionary_remove(task_aux->data,"ftp_id");
			ftp_ids_len--;
		}
	}
	task_destroy(&task_aux);
	if(!ok){	
		task_done(t,HTTP_500,"Error fatal a analizar");
		return 0;
	}

	if(!db_get_hash_dir(db,site_id,hash_dir,site_name,error,&db_fail)){
		if(db_fail){
			task_done(t,HTTP_501,M_DB_ERROR);
		} else {
			task_done(t,HTTP_499,error);
		}
		return 0;
	}

	printf("Tenemos el hash\n");
	/* Borramos de la base de datos el sitio, indices, alias,
 	   usuarios ftp y recuperamos espacio en disco */
	sprintf(command,"du -bs /%s/%s/%s",config_webdir(c),hash_dir,site_name);
	SYSTEM_DO
	if(!db_site_del(db,site_id,size,error,&db_fail)){
		task_done(t,HTTP_500,"Error indefinido");
		return 0;
	}
	printf("Tenemos entradas en la DB\n");

	/* Lo borramos logicamente y quitamos del apache */
	site = lista_find(l,site_get_id,atoi(site_id));
	if(site){
		/* Siempre deberiamos entrar por true. Pero...
 		 * si hay algún error donde site = NULL debemos tener
 		 * cuidado */
		lista_first(site_get_workers(site));
		while(!lista_eol(site_get_workers(site))){
			worker = lista_get(site_get_workers(site));
			worker_remove_site(worker,site);
			lista_next(site_get_workers(site));
		}
	}
	printf("Quitamos de los apaches y listas\n");

	/* Lo borramos fisicamente de la estructura de directorios */
	sprintf(command,"rm -rf /websites/%s/%s",hash_dir,site_name);
	SYSTEM_DO

	printf("Borramos del directorio\n");

	task_done(t,HTTP_200,"sitio borrados");
	logs_write(logs,L_INFO,"task_site_del","Sitio borrado");
	return 1;
}

void task_namespace_list(T_task *t, T_db *db, T_logs *logs){
	/* Lista todos los namespaces */
	char *message=NULL;
	int db_fail;

	db_namespace_list(db,&message,&db_fail);
	if(db_fail)
		task_done(t,HTTP_501,M_DB_ERROR);
	else
	    task_done(t,HTTP_200,message);
}

int task_namespace_show(T_task *t, T_db *db, T_logs *logs){
	/* Retorna en formato Json informacion de la suscripcion */
	int db_fail;
	char *message=NULL;

	printf("Task namespace_show\n");
	if(!db_namespace_show(db,dictionary_get(t->data,"namespace_id"),&message,&db_fail)){
		printf("namespace_show 1\n");
		if(db_fail)
			task_done(t,HTTP_500,"Error fatal a analizar: task_namespace_show");
		else
			task_done(t,HTTP_404,message);
	} else {
		task_done(t,HTTP_200,message);
	}
}

void task_namespace_add(T_task *t, T_db *db, T_logs *logs){
	/* Agrega una suscripcion */
	int db_fail;

	if(db_namespace_add(db,t->data,&db_fail)){
		task_done(t,HTTP_200,"Namespace agregado");
	} else {
		task_done(t,HTTP_500,"Error fatal a analizar: task_namespace_add");
	}
}

void task_namespace_del(T_task *t, T_lista *l, T_db *db, T_config *c, T_logs *logs){
	/* Elimina un namespace todos sus datos */
	/* Retorna 1 si pudo borrar todo. Caso contrario retorna 0 */

	int site_ids[256];	//256 es la maxima cantidad de sitios por namespace
	int site_ids_len = 0;	//Cantidad de elementos del array site_ids
	T_task *task_aux=NULL;
	T_dictionary *data_aux=NULL;
	int ok=1;
	char site_id[100];
	char *namespace_id;
	char error[200];
	int db_fail;

	namespace_id = dictionary_get(t->data,"namespace_id");
	printf("task_namespace_del\n");
	task_aux = (T_task *)malloc(sizeof(T_task));
	printf("task_namespace_del\n");
	data_aux = (T_dictionary *)malloc(sizeof(T_dictionary));
	dictionary_init(data_aux);
	task_init(task_aux);
	task_set(task_aux,T_SITE_DEL,data_aux);
	if(!db_get_sites_id(db,namespace_id,site_ids,&site_ids_len,error,&db_fail)){
		if(db_fail)
			task_done(t,HTTP_501,M_DB_ERROR);
		else
			task_done(t,HTTP_499,error);
	} else {
		while((site_ids_len > 0) && ok){
			sprintf(site_id,"%i",site_ids[site_ids_len - 1 ]);
			dictionary_add(task_aux->data,"site_id",site_id);
			dictionary_print(task_aux->data);
			ok &= task_site_del(task_aux,l,db,c,logs);
			dictionary_remove(task_aux->data,"site_id");
			site_ids_len--;
		}
	}
	task_destroy(&task_aux);
	printf("OK vale=%i\n",ok);
	if(ok){
		/* Eliminados los sitios borramos la suscripcion de la base de datos */
		if(!db_namespace_del(db,namespace_id))
			task_done(t,HTTP_500,"Error fatal a analizar: task_namespace_del:1");
		else
			task_done(t,HTTP_200,"Namespace eliminado");
	} else
		task_done(t,HTTP_500,"Error fatal a analizar: task_namespace_del:2");
}

void task_namespace_stop(T_task *t, T_lista *l, T_db *db){
	/* Detiene todos los sitios de una suscripcion */
	int site_ids[256];	//256 es la maxima cantidad de sitios por suscripcion
	int site_ids_len = 0;	//Cantidad de elementos del array site_ids
	T_task *task_aux;
	char *namespace_id;
	char site_id[50];
	char error[200];
	int db_fail;
	int ok;

	printf("task_namespace_stop\n");
	task_aux = (T_task *)malloc(sizeof(T_task));
	namespace_id = dictionary_get(t->data,"namespace_id");
	if(strcmp(namespace_id,"") != 0){
		if(!db_get_sites_id(db,namespace_id,site_ids,&site_ids_len,error,&db_fail)){
			if(db_fail)
				task_done(t,HTTP_501,M_DB_ERROR);
			else
				task_done(t,HTTP_499,error);
		} else {
			while((site_ids_len > 0) && ok){
				sprintf(site_id,"%i",site_ids[site_ids_len-1]);
				dictionary_add(task_aux->data,"site_id",site_id);
				ok &= task_site_stop(task_aux,l,db);
				dictionary_remove(task_aux->data,"site_id");
				site_ids_len--;
			}
		}
		free(task_aux);
		if(ok)
			task_done(t,HTTP_200,"Namespace detenido");
		else
			task_done(t,HTTP_500,"Error fatal a analizar: task_namespace_stop:1");
	} else
		task_done(t,HTTP_500,"Error fatal a analizar: task_namespace_stop:2");
}
	
void task_namespace_start(T_task *t, T_lista *l, T_db *db){
	/* ARRANCA todos los sitios de una suscripcion */
	T_task *task_aux;
	int site_ids[256];	//256 es la maxima cantidad de sitios por suscripcion
	int site_ids_len = 0;	//Cantidad de elementos del array site_ids
	char *namespace_id;
	char site_id[50];
	char error[200];
	int db_fail;
	int ok;

	printf("task_namespace_start\n");
	task_aux = (T_task *)malloc(sizeof(T_task));
	namespace_id = dictionary_get(t->data,"namespace_id");
	if(strcmp(namespace_id,"") != 0){
		if(!db_get_sites_id(db,namespace_id,site_ids,&site_ids_len,error,&db_fail)){
			if(db_fail)
				task_done(t,HTTP_501,M_DB_ERROR);
			else
				task_done(t,HTTP_499,error);
		} else {
			while((site_ids_len > 0) && ok){
				sprintf(site_id,"%i",site_ids[site_ids_len-1]);
				dictionary_add(task_aux->data,"site_id",site_id);
				ok &= task_site_start(task_aux,l,db);
				dictionary_remove(task_aux->data,"site_id");
			}
		}
		free(task_aux);
		if(ok)
			task_done(t,HTTP_200,"Namespace iniciado");
		else
			task_done(t,HTTP_500,"Error fatal a analizar: task_namespace_start:1");
	} else
		task_done(t,HTTP_500,"Error fatal a analizar: task_namespace_start:2");
}

int task_site_stop(T_task *t, T_lista *l, T_db *db, T_logs *logs){
	/* Coloca un sitio offline */
	char error[200];
	int db_fail;
	T_site *site;

	if(!db_site_status(db,dictionary_get(t->data,"namespace_id"),
	   dictionary_get(t->data,"site_id"),
	   dictionary_get(t->data,"status"),error,&db_fail)){
		if(db_fail)
			task_done(t,HTTP_501,M_DB_ERROR);
		else
			task_done(t,HTTP_499,error);
		return 0;
	}
	/* Ya cambiado en la base de datos... procedemos */
	site = lista_find(l,site_get_id,atoi(dictionary_get(t->data,"site_id")));
	site_stop(site);
	return 1;
}

int task_site_start(T_task *t, T_lista *l, T_db *db, T_logs *logs){
	/* Coloca un sitio online */
	char error[200];
	int db_fail;
	T_site *site;

	if(!db_site_status(db,dictionary_get(t->data,"namespace_id"),
	   dictionary_get(t->data,"site_id"),
	   dictionary_get(t->data,"status"),error,&db_fail)){
		if(db_fail)
			task_done(t,HTTP_501,M_DB_ERROR);
		else
			task_done(t,HTTP_499,error);
		return 0;
	}
	/* Ya cambiado en la base de datos... procedemos */
	site = lista_find(l,site_get_id,atoi(dictionary_get(t->data,"site_id")));
	site_start(site);
	return 1;
}

void task_site_mod(T_task *t, T_lista *l, T_db *db, T_logs *logs){
	/* Modifica un sitio */

	T_site *site;
	int db_fail;
	uint16_t version;
	char error[200];
	char aux[40];

	/* Verificamos que el sitio corresponda al suscriber_id */
	version = db_site_exist(db,dictionary_get(t->data,"namespace_id"),
		dictionary_get(t->data,"site_id"),error,&db_fail);
	if(!version){
		if(db_fail){
			task_done(t,HTTP_501,M_DB_ERROR);
		} else {
			task_done(t,HTTP_404,"Sitio no existe\"");
		}
		return;
	}
	/* Verificamos que el sitio exista en las estructuras */
	site = lista_find(l,site_get_id,atoi(dictionary_get(t->data,"site_id")));
	if(!site){
		task_done(t,HTTP_500,"Error fatal a analizar: task_site_mod");
		return;
	}

	sprintf(aux,"%lu",version);
	dictionary_add(t->data,"version",aux);
	/* modificamos el sitio en la base de datos */
	if(!db_site_mod(db,site,t->data,error,&db_fail)){
		if(db_fail)
			task_done(t,HTTP_501,M_DB_ERROR);
		else{
			task_done(t,HTTP_499,error);
		}
	}
	task_done(t,HTTP_200,"Sitio modificado");
}

/*************************************************
 * 		TASK FTP
 *************************************************/

void task_ftp_list(T_task *t, T_db *db){

	if(!db_ftp_list(db,&(t->result),dictionary_get(t->data,"site_id"))){
		task_done(t,HTTP_500,"Error fatal a analizar: task_ftp_list");
	}
	printf("Termino ftp\n");
}

void task_ftp_add(T_task *t, T_db *db, T_config *config){
	/* Agrega usuario secundario al sitio */
	char error[200];
	int db_fail;

	/* Verificamos que no se supere el limite de usuarios
	   ftp por sitio */
	if(0 == db_limit_ftp_users(db, dictionary_get(t->data,"site_id"), &db_fail)){
		if(db_fail)
			task_done(t,HTTP_501,M_DB_ERROR);
		else
			task_done(t,HTTP_420,M_FTP_LIMT_ERROR);
	} else {
		if(!db_ftp_add(db,t->data,config,error,&db_fail)){
			if(db_fail){
				task_done(t,HTTP_501,M_DB_ERROR);
			} else {
				task_done(t,HTTP_499,error);
			}
		} else
			task_done(t,HTTP_200,"Usuario ftp agregado");
	}
}

int task_ftp_del(T_task *t, T_db *db){
	/* Elimina usuario ftp del sitio. Si no pudo retorna 0.
 	   Si pudo retorna 1. */
	char error[200];
	int db_fail;

	if(!db_ftp_del(db,t->data,error,&db_fail)){
		if(db_fail)
			task_done(t,HTTP_501,M_DB_ERROR);
		else
			task_done(t,HTTP_499,error);
		return 0;
	} else {
		task_done(t,HTTP_200,"Usuario ftp eliminado");
		return 1;
	}
}

void task_ftp_mod(T_task *t, T_db *db){
	/* Modifica parte del nombre y pass del usuario
	 * sea principal o secundario */
	/* IMPLEMENTAR */
}

/*************************************************
 * 		TASK SERVER
 *************************************************/

void task_server_list(T_task *t, T_db *db, T_lista *lw, T_lista *lp){
	char *message;
	json_servers(&message,lw,lp,db);
	task_done(t,HTTP_200,message);
}

void task_server_show(T_task *t, T_db *db, T_lista *lw, T_lista *lp){
	/* Retorna informacion especifica de un server en particular */
	char *id;
	char *message;
	T_worker *worker = NULL;
	T_proxy *proxy = NULL;

	id = dictionary_get(t->data,"server_id");
	printf("TASK_SERVER_SHOW: Info del server id=%s\n",id);
	/* Posiblemente sea un worker */
	worker = lista_find(lw,worker_get_id,atoi(id));
	if(worker){
		printf("Es un worker: %p\n",worker);
		if(!json_worker(&message,worker,db))
			task_done(t,HTTP_501,M_DB_ERROR);
		else
			task_done(t,HTTP_200,message);
	} else {
		/* Posiblemente sea un proxy */
		proxy = lista_find(lp,proxy_get_id,atoi(id));
		if(proxy){
			printf("Es un proxy\n");
			if(!json_proxy(&message,proxy,db))
				task_done(t,HTTP_501,M_DB_ERROR);
			else
				task_done(t,HTTP_200,message);
		} else {
			printf("Server con id:%s no existe\n",id);
			task_done(t,HTTP_404,"Servidor inexistente");
		}
	}
}

int task_server_stop(T_task *t, T_db *db, T_lista *lw, T_lista *lp){
	/* Detenemos el worker y lo indicamos en
 	 * la base de datos */
	char *id;
	T_worker *worker = NULL;
	T_proxy *proxy = NULL;

	id = dictionary_get(t->data,"server_id");
	printf("TASK_SERVER_STOP: Deteniendo server id=%s\n",id);
	worker = lista_find(lw,worker_get_id,atoi(id));
	if(worker){
		worker_stop(worker);
		task_done(t,HTTP_200,"Servidor worker detenido");
	} else {
		proxy = lista_find(lp,proxy_get_id,atoi(id));
		if(proxy){
			proxy_stop(proxy);
			task_done(t,HTTP_200,"Servidor proxy detenido");
		} else {
			task_done(t,HTTP_404,"Servidor inexistente");
		}
	}
}

int task_server_start(T_task *t, T_db *db, T_lista *lw, T_lista *lp){
	/* Arrancamos el worker y lo indicamos en
 	 * la base de datos */
	char *id;
	T_worker *worker = NULL;
	T_proxy *proxy = NULL;

	id = dictionary_get(t->data,"server_id");
	printf("TASK_SERVER_START: Iniciando server id=%s\n",id);
	worker = lista_find(lw,worker_get_id,atoi(id));
	if(worker){
		worker_start(worker);
		task_done(t,HTTP_200,"Servidor worker iniciado");
	} else {
		proxy = lista_find(lp,proxy_get_id,atoi(id));
		if(proxy){
			proxy_stop(proxy);
			task_done(t,HTTP_200,"Servidor proxy iniciado");
		} else {
			task_done(t,HTTP_404,"Servidor inexistente");
		}
	}
}

void task_run(T_task *t, T_lista *sites, T_lista *workers,
		T_lista *proxys, T_db *db, T_config *config, T_logs *logs){
	/* Ejecuta el JOB */
	t->status = T_RUNNING;

	printf("TASK_RUN: %i\n",t->type);
	switch(t->type){
		/* lOGIN */
		case T_LOGIN:
			task_login(t,db);
			break;

		/* Namespaces */
		case T_NAMESPACE_LIST:
			task_namespace_list(t,db,logs); break;
		case T_NAMESPACE_ADD:
			task_namespace_add(t,db,logs); break;
		case T_NAMESPACE_DEL:
			task_namespace_del(t,sites,db,config,logs); break;
		case T_NAMESPACE_STOP:
			task_namespace_stop(t,sites,db); break;
		case T_NAMESPACE_START:
			task_namespace_start(t,sites,db); break;
		case T_NAMESPACE_SHOW:
			task_namespace_show(t,db,logs); break;

		/* SITIOS */
		case T_SITE_LIST:
			task_site_list(t,db,logs);break;
		case T_SITE_SHOW:
			task_site_show(t,db,logs); break;
		case T_SITE_ADD:
			task_site_add(t,sites,db,config,logs); break;
		case T_SITE_DEL:
			task_site_del(t,sites,db,config,logs); break;
		case T_SITE_MOD:
			task_site_mod(t,sites,db,logs); break;
		case T_SITE_STOP:
			task_site_stop(t,sites,db,logs); break;
		case T_SITE_START:
			task_site_start(t,sites,db,logs); break;

		/* Servers */
		case T_SERVER_LIST:
			task_server_list(t,db,workers,proxys); break;
		case T_SERVER_SHOW:
			task_server_show(t,db,workers,proxys); break;
		case T_SERVER_STOP:
			task_server_stop(t,db,workers,proxys); break;
		case T_SERVER_START:
			task_server_start(t,db,workers,proxys); break;

		/* FTP access */
		case T_FTP_LIST:
			task_ftp_list(t,db); break;
		case T_FTP_ADD:
			task_ftp_add(t,db,config); break;
		case T_FTP_DEL:
			task_ftp_del(t,db); break;
		case T_FTP_MOD:
			task_ftp_mod(t,db); break;

		default:
			printf("ERROR FATAL. TASK_TYPE indefinido:%i\n",t->type);

	}
}

/*****************************
	  Cola de Jobs
 ******************************/

void heap_task_init(T_heap_task *h){
	h->first = NULL;
	h->last = NULL;
	h->size = 0;
}

void heap_task_push(T_heap_task *h, T_task *t){
	heap_t_node *new;
	heap_t_node *aux;

	printf("heap_task_push\n");
	new = (heap_t_node*)malloc(sizeof(heap_t_node));
	new->next = NULL;
	new->data = t;
	h->size++;

	if(h->first == NULL){
		h->first = new;
		h->last = new;
	} else {
		h->last->next = new;
		h->last = new;
	}
}

T_task *heap_task_pop(T_heap_task *h){
	heap_t_node *aux;
	T_task *taux;

	if(h->first != NULL){
		aux = h->first;
		taux = h->first->data;
		h->first = h->first->next;
		if(h->first == NULL)
			h->last = NULL;
		free(aux);
		return taux;
	} else {
		return NULL;
	}
}

unsigned int heap_task_size(T_heap_task *h){
	return h->size;
}

T_task *heap_task_exist(T_heap_task *h, T_taskid id){
	/* si existe el id de task se retorna. Caso contrario retorna NULL */
	heap_t_node *aux;
	T_task *exist = NULL;
	
	aux = h->first;
	while(exist == NULL && aux!= NULL){
        if(strcmp(task_get_id(aux->data),id) == 0)
            exist = aux->data;
        aux = aux->next;
    }
	return exist;
}

void heap_task_print(T_heap_task *h){

	heap_t_node *aux;

	aux = h->first;
	while(aux!= NULL){
		printf("Job_ID: %s\n",task_get_id(aux->data));
		aux = aux->next;
	}
}

/******************************
 * 		BAG JOB
 ******************************/

void bag_task_init(T_bag_task *b){
	b->first = NULL;
	b->actual = NULL;
	b->last = NULL;
	b->size = 0;
}

void bag_task_add(T_bag_task *b, T_task *t){
	bag_t_node *new;
	bag_t_node *aux;

	printf("bag_task_add\n");
	new = (bag_t_node*)malloc(sizeof(bag_t_node));
	new->next = NULL;
	new->data = t;
	b->size++;

	if(b->first == NULL){
		b->first = new;
		b->last = new;
	} else {
		b->last->next = new;
		b->last = new;
	}
}

T_task *bag_site_remove(T_bag_task *b){
	bag_t_node *prio;
	bag_t_node *aux;
	T_task *element = NULL;

	if(b->actual != NULL){
		aux = b->first;
		prio = NULL;
		while(aux != b->actual){
			prio = aux;
			aux = aux->next;
		}
		if(prio == NULL){
			b->first = aux->next;
		} else {
			prio->next = aux->next;
		}
		if(aux == b->last){
			b->last = prio;
		}
		b->actual = aux->next;
		element = aux->data;
		free(aux);
	}
	return element;
}

T_task *bag_task_pop(T_bag_task *b, T_taskid *id){
	/* Retorna el task buscado por su id.
 	 * si no existe retorna NULL. Si existe
 	 * no solo lo retorna sino que lo elimina
 	 * de la bolsa */
	int exist = 0;
	T_task *taux = NULL;

	b->actual = b->first;
	while((b->actual != NULL) && !exist){
		printf("Comparamos -%s- con -%s-\n",id,task_get_id(b->actual->data));
		exist = (strcmp(task_get_id(b->actual->data),(char *)id)==0);
		if((!exist && (b->actual != NULL))){
			printf("	Avanzamos. No son iguales\n");
			b->actual = b->actual->next;
		}
		//sleep(5);
	}
	if(exist)
		taux = bag_site_remove(b);
	return taux;
}

unsigned int bag_task_size(T_bag_task *b){
	return b->size;
}

void bag_task_timedout(T_bag_task *b, int d){
	/* Elimina de la estructura todas las
	   tareas cuya diferencia de tiempo entre
	   finalizado y la hora actual sea lo que
	   se indica en el parametro d */

	T_task *taux = NULL;
	time_t now = time(NULL);

	//printf("bag_task_timedout\n");
	b->actual = b->first;
	while((b->actual != NULL)){
		if(d < difftime(now,task_get_time(b->actual->data))){
			taux = bag_site_remove(b);
			free(taux);
		} else
			b->actual = b->actual->next;
	}
}

void bag_task_print(T_bag_task *b){

	bag_t_node *aux;

	printf("PRINT BAG\n");
	aux = b->first;
	while(aux!= NULL){
		printf("Job_ID: %s\n",task_get_id(aux->data));
		aux = aux->next;
	}
	printf("END PRINT BAG\n");
}
