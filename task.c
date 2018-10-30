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

T_task_type task_c_to_type(char c){
	switch(c){
		case '0': return T_SUSC_ADD;
		case '1': return T_SUSC_DEL;
		case '2': return T_SUSC_STOP;
		case '3': return T_SUSC_START;
		case '4': return T_SUSC_SHOW;

		case 'l': return T_SITE_LIST;
		case 's': return T_SITE_SHOW;
		case 'a': return T_SITE_ADD;
		case 'm': return T_SITE_MOD;
		case 'd': return T_SITE_DEL;
		case 'k': return T_SITE_STOP;
		case 'e': return T_SITE_START;

		case 'L': return T_SERVER_LIST;
		case 'S': return T_SERVER_SHOW;
		case 'A': return T_SERVER_ADD;
		case 'M': return T_SERVER_MOD;
		case 'D': return T_SERVER_DEL;
		case 'K': return T_SERVER_STOP;
		case 'E': return T_SERVER_START;
	}
}


/*****************************
	     TASK 
******************************/
void task_init(T_task *t, T_task_type type, T_dictionary *data){
	random_task_id(t->id);
	//random_token(t->token);
	t->type = type;
	t->data = data;
	t->result = NULL;
	t->result_size = 0;
}

void task_destroy(T_task **t){
	if((*t)->data != NULL){
		dictionary_destroy(&((*t)->data));
	}
	free((*t)->result);
	free(*t);
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

void task_done(T_task *t, char *message){
	t->status = T_DONE;
	t->result_size = strlen(message) + 1;	// El +1 es lara el "\0" del final del string
	printf("Allocamos espacio\n");
	t->result=(char *)realloc(t->result,t->result_size);
	printf("Copiamos mensaje -%s-\n",message);
	strcpy(t->result,message);
}

void task_site_list(T_task *t, T_db *db, T_logs *logs){
	/* Lista sitios de una suscripcion dada */
	char *susc_id;

	susc_id = dictionary_get(t->data,"susc_id");
	db_site_list(db,&(t->result),&(t->result_size),susc_id);
	/* Falta agregar una lista de los workers donde esta
 	   cada site */
}

void task_site_show(T_task *t, T_db *db, T_logs *logs){
	char *site_id;
	char *susc_id;
	T_site *site;

	site_id = dictionary_get(t->data,"site_id");
	susc_id = dictionary_get(t->data,"susc_id");
	db_site_show(db,&(t->result),&(t->result_size),site_id,susc_id);
}

int task_site_add(T_task *t, T_list_site *l, T_db *db, T_logs *logs){
	/* Agrega un sitio a la solucion */

	T_site *newsite;
	char error[200];
	int db_fail;
	char sql[300];
	char *name;
	char *susc_id;
	char command[300];
	char hash_dir[6];
	unsigned int id;
	
	printf("Agregar un sitio nuevo\n");
	dictionary_print(t->data);
	name = dictionary_get(t->data,"name");
	susc_id = dictionary_get(t->data,"susc_id");

	// Alta en la base de datos del sitio
	if(!db_site_add(db,&newsite,name,atoi(susc_id),hash_dir,error,&db_fail)){
		if(db_fail){
			task_done(t,ERROR_FATAL);
			logs_write(logs,L_ERROR,"task_site_add", "DB_ERROR");
		} else {
			task_done(t,error);
		}
		printf("Paso\n");
		return 0;
	}

	// Creacion del espacio de almacenamiento
	printf("Creacion de directorios\n");
	sprintf(command,"mkdir -p /websites/%s/%s/wwwroot",hash_dir,name);
	if(system(command) != 0){
		task_done(t,"300|\"code\":\"300\",\"info\":\"ERROR FATAL\"");
		return 0;
	}
	sprintf(command,"mkdir -p /websites/%s/%s/logs",hash_dir,name);
	if(system(command) != 0){
		task_done(t,"300|\"code\":\"300\",\"info\":\"ERROR FATAL\"");
		return 0;
	}

	// Adicion del sitio a la lista
	printf("agregado al listado\n");
	list_site_add(l,newsite);

	task_done(t,"200|\"code\":\"201\",\"info\":\"sitio agregado correctamente\"");
	return 1;
}

int task_site_del(T_task *t, T_list_site *l, T_db *db, T_logs *logs){
	/* Borra fisicamente y logicamente un sitio.
 	 * si pudo borrarlo retorna 1. Si no pudo o
 	 * fue borrado parcialmente retorna 0 */
	char hash_dir[10];
	char error[200];
	char site_name[100];
	char command[200];
	char *site_id;
	int db_fail;
	T_site *site = NULL;
	T_worker *worker;

	printf("task_site_del\n");

	site_id = dictionary_get(t->data,"site_id");
	printf("pasamos %s\n",site_id);

	if(!db_get_hash_dir(db,site_id,hash_dir,site_name,error,&db_fail)){
		if(db_fail){
			task_done(t,ERROR_FATAL);
		} else {
			task_done(t,error);
		}
		return 0;
	}
	printf("Tenemos el hash\n");
	/* Borramos de la base de datos el sitio, indices y alias*/
	if(!db_site_del(db,site_id,error,&db_fail)){
		task_done(t,ERROR_FATAL);
		return 0;
	}
	printf("Tenemos entradas en la DB\n");

	/* Lo borramos logicamente y quitamos del apache */
	site = list_site_find_id(l,atoi(site_id));
	if(site){
		/* Siempre deberiamos entrar por true. Pero...
 		 * si hay algún error donde site = NULL debemos tener
 		 * cuidado */
		list_worker_first(site_get_workers(site));
		while(!list_worker_eol(site_get_workers(site))){
			worker = list_worker_get(site_get_workers(site));
			worker_remove_site(worker,site);
			list_worker_next(site_get_workers(site));
		}
	}
	printf("Quitamos de los apaches y listas\n");

	/* Lo borramos fisicamente de la estructura de directorios */
	sprintf(command,"rm -rf /websites/%s/%s",hash_dir,site_name);
	logs_write(logs,L_DEBUG,"task_site_del",command);
	if(system(command) != 0){
		logs_write(logs,L_ERROR,"task_site_del","Imposible borrar el directorio");
		task_done(t,ERROR_FATAL);
		return 0;
	}
	printf("Borramos del directorio\n");

	task_done(t,"200|\"code\":\"202\",\"info\":\"Sitio borrado\"");
	logs_write(logs,L_INFO,"task_site_del","Sitio borrado");
	return 1;
}


int task_susc_show(T_task *t, T_db *db, T_logs *logs){
	/* Retorna en formato Json informacion de la suscripcion */
	int db_fail;
	char *message=NULL;

	if(db_susc_show(db,dictionary_get(t->data,"susc_id"),&message,&db_fail)){
		task_done(t,message);
	} else {
		task_done(t,"300|\"code\":\"300\",\"info\":\"ERROR FATAL\"");
	}
	
}

int task_susc_add(T_task *t, T_db *db, T_logs *logs){
	/* Agrega una suscripcion */
	int db_fail;

	if(db_susc_add(db,dictionary_get(t->data,"susc_id"),&db_fail)){
		task_done(t,"200|\"code\":\"202\",\"info\":\"Suscripcion Agregada\"");
	} else {
		task_done(t,"300|\"code\":\"300\",\"info\":\"ERROR FATAL\"");
	}
}

void task_susc_del(T_task *t, T_list_site *l, T_db *db, T_logs *logs){
	/* Elimina una suscripcion y todos sus datos */
	/* Retorna 1 si pudo borrar todo. Caso contrario retorna 0 */
	int site_ids[256];	//256 es la maxima cantidad de sitios por suscripcion
	int site_ids_len = 0;	//Cantidad de elementos del array site_ids
	T_task *task_aux=NULL;
	T_dictionary *data_aux=NULL;
	int ok=1;
	char site_id[100];
	char *susc_id;
	char error[200];
	int db_fail;

	susc_id = dictionary_get(t->data,"susc_id");
	if(strcmp(susc_id,"") != 0){
		printf("task_susc_del\n");
		task_aux = (T_task *)malloc(sizeof(T_task));
		printf("task_susc_del\n");
		data_aux = (T_dictionary *)malloc(sizeof(T_dictionary));
		dictionary_init(data_aux);
		task_init(task_aux,T_SITE_DEL,data_aux);
		if(!db_get_sites_id(db,susc_id,site_ids,&site_ids_len,error,&db_fail)){
			if(db_fail)
				task_done(t,ERROR_FATAL);
			else
				task_done(t,error);
		} else {
			while((site_ids_len > 0) && ok){
				sprintf(site_id,"%i",site_ids[site_ids_len - 1 ]);
				dictionary_add(task_aux->data,"site_id",site_id);
				dictionary_print(task_aux->data);
				ok &= task_site_del(task_aux,l,db,logs);
				dictionary_remove(task_aux->data,"site_id");
				site_ids_len--;
			}
		}
		task_destroy(&task_aux);
		printf("OK vale=%i\n",ok);
		if(ok){
			/* Eliminados los sitios borramos la suscripcion de la base de datos */
			if(!db_susc_del(db,susc_id))
				task_done(t,ERROR_FATAL);
			else
				task_done(t,"200");
		} else
			task_done(t,"300|\"code\":\"300\",\"info\":\"ERROR FATAL\"");
	} else 
		task_done(t,"300|\"code\":\"300\",\"info\":\"ERROR FATAL\"");
}

void task_susc_stop(T_task *t, T_list_site *l, T_db *db){
	/* Detiene todos los sitios de una suscripcion */
	int site_ids[256];	//256 es la maxima cantidad de sitios por suscripcion
	int site_ids_len = 0;	//Cantidad de elementos del array site_ids
	T_task *task_aux;
	char *susc_id;
	char site_id[50];
	char error[200];
	int db_fail;
	int ok;

	printf("task_susc_stop\n");
	task_aux = (T_task *)malloc(sizeof(T_task));
	susc_id = dictionary_get(t->data,"susc_id");
	if(strcmp(susc_id,"") != 0){
		if(!db_get_sites_id(db,susc_id,site_ids,&site_ids_len,error,&db_fail)){
			if(db_fail)
				task_done(t,ERROR_FATAL);
			else
				task_done(t,error);
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
			task_done(t,"200");
		else
			task_done(t,"300|\"code\":\"300\",\"info\":\"ERROR FATAL\"");
	} else
		task_done(t,"300|\"code\":\"300\",\"info\":\"ERROR FATAL\"");
}
	
void task_susc_start(T_task *t, T_list_site *l, T_db *db){
	/* ARRANCA todos los sitios de una suscripcion */
	T_task *task_aux;
	int site_ids[256];	//256 es la maxima cantidad de sitios por suscripcion
	int site_ids_len = 0;	//Cantidad de elementos del array site_ids
	char *susc_id;
	char site_id[50];
	char error[200];
	int db_fail;
	int ok;

	printf("task_susc_start\n");
	task_aux = (T_task *)malloc(sizeof(T_task));
	susc_id = dictionary_get(t->data,"susc_id");
	if(strcmp(susc_id,"") != 0){
		if(!db_get_sites_id(db,susc_id,site_ids,&site_ids_len,error,&db_fail)){
			if(db_fail)
				task_done(t,ERROR_FATAL);
			else
				task_done(t,error);
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
			task_done(t,"200");
		else
			task_done(t,"300|\"code\":\"300\",\"info\":\"ERROR FATAL\"");
	} else
		task_done(t,"300|\"code\":\"300\",\"info\":\"ERROR FATAL\"");
}

int task_site_stop(T_task *t, T_list_site *l, T_db *db, T_logs *logs){
	/* Coloca un sitio offline */
	char error[200];
	int db_fail;
	T_site *site;

	if(!db_site_status(db,dictionary_get(t->data,"susc_id"),
	   dictionary_get(t->data,"site_id"),
	   dictionary_get(t->data,"status"),error,&db_fail)){
		if(db_fail)
			 task_done(t,ERROR_FATAL);
		else
			 task_done(t,error);
		return 0;
	}
	/* Ya cambiado en la base de datos... procedemos */
	site = list_site_find_id(l,atoi(dictionary_get(t->data,"site_id")));
	site_stop(site);
	return 1;
}

int task_site_start(T_task *t, T_list_site *l, T_db *db, T_logs *logs){
	/* Coloca un sitio online */
	char error[200];
	int db_fail;
	T_site *site;

	if(!db_site_status(db,dictionary_get(t->data,"susc_id"),
	   dictionary_get(t->data,"site_id"),
	   dictionary_get(t->data,"status"),error,&db_fail)){
		if(db_fail)
			 task_done(t,ERROR_FATAL);
		else
			 task_done(t,error);
		return 0;
	}
	/* Ya cambiado en la base de datos... procedemos */
	site = list_site_find_id(l,atoi(dictionary_get(t->data,"site_id")));
	site_start(site);
	return 1;
}

void task_site_mod(T_task *t, T_list_site *l, T_db *db, T_logs *logs){
	/* Modifica un sitio */

	T_site *site;
	int db_fail;
	uint16_t version;
	char error[200];
	char aux[40];

	printf("Modificamos un sitio\n");
        dictionary_print(t->data);

	/* Verificamos que el sitio corresponda al suscriber_id */
	version = db_site_exist(db,dictionary_get(t->data,"susc_id"),
		dictionary_get(t->data,"site_id"),error,&db_fail);
	if(!version){
		if(db_fail){
			task_done(t,ERROR_FATAL);
		} else {
			task_done(t,"300|\"code\":\"302\",\"info\":\"Sitio no existe\"");
		}
		return;
	}
	/* Verificamos que el sitio exista en las estructuras */
	site = list_site_find_id(l,atoi(dictionary_get(t->data,"site_id")));
	if(!site){
		task_done(t,ERROR_FATAL);
		return;
	}

	sprintf(aux,"%lu",version);
	dictionary_add(t->data,"version",aux);
	/* modificamos el sitio en la base de datos */
	if(!db_site_mod(db,site,t->data,error,&db_fail)){
		if(db_fail)
			task_done(t,ERROR_FATAL);
		else{
			task_done(t,error);
		}
	}
	task_done(t,"200|\"code\":\"203\",\"info\":\"Sitio modificado\"");
}

/*************************************************
 * 		TASK SERVER
 *************************************************/

void task_server_list(T_task *t, T_db *db, T_list_worker *lw, T_list_proxy *lp){
	json_servers(&(t->result),&(t->result_size),lw,lp,db);
}

void task_server_show(T_task *t, T_db *db, T_list_worker *lw, T_list_proxy *lp){
	/* Retorna informacion especifica de un server en particular */
	char *id;
	T_worker *worker = NULL;
	T_proxy *proxy = NULL;

	id = dictionary_get(t->data,"id");
	worker = list_worker_find_id(lw,atoi(id));
	if(worker){
		if(!json_worker(&(t->result),&(t->result_size),worker,db)){
			task_done(t,"300|\"code\":\"300\",\"info\":\"ERROR base de datos\"");
		}
	} else {
		proxy = list_proxy_find_id(lp,atoi(id));
		if(proxy){
			if(!json_proxy(&(t->result),&(t->result_size),proxy,db)){
				task_done(t,"300|\"code\":\"300\",\"info\":\"ERROR base de datos\"");
			}
		} else {
			task_done(t,"300|\"code\":\"310\",\"info\":\"Server no existe\"");
		}
	}
}

int task_server_stop(T_task *t, T_db *db, T_list_worker *lw, T_list_proxy *lp){
	/* Detenemos el worker y lo indicamos en
 	 * la base de datos */
	char *id;
	T_worker *worker = NULL;
	T_proxy *proxy = NULL;

	id = dictionary_get(t->data,"id");
	worker = list_worker_find_id(lw,atoi(id));
	if(worker){
		worker_stop(worker);
	} else {
		proxy = list_proxy_find_id(lp,atoi(id));
		if(proxy){
			proxy_stop(proxy);
		} else {
			task_done(t,"300|\"code\":\"310\",\"info\":\"Server no existe\"");
		}
	}
}

int task_server_start(T_task *t, T_db *db, T_list_worker *lw, T_list_proxy *lp){
	/* Arrancamos el worker y lo indicamos en
 	 * la base de datos */
	char *id;
	T_worker *worker = NULL;
	T_proxy *proxy = NULL;

	id = dictionary_get(t->data,"id");
	worker = list_worker_find_id(lw,atoi(id));
	if(worker){
		worker_start(worker);
	} else {
		proxy = list_proxy_find_id(lp,atoi(id));
		if(proxy){
			proxy_stop(proxy);
		} else {
			task_done(t,"300|\"code\":\"310\",\"info\":\"Server no existe\"");
		}
	}
}

void task_run(T_task *t, T_list_site *sites, T_list_worker *workers,
		T_list_proxy *proxys, T_db *db, T_logs *logs){
	/* Ejecuta el JOB */
	t->status = T_RUNNING;

	printf("TASK_RUN: %i\n",t->type);
	switch(t->type){
		case T_SUSC_ADD:
			task_susc_add(t,db,logs); break;
		case T_SUSC_DEL:
			task_susc_del(t,sites,db,logs); break;
		case T_SUSC_STOP:
			task_susc_stop(t,sites,db); break;
		case T_SUSC_START:
			task_susc_start(t,sites,db); break;
		case T_SUSC_SHOW:
			task_susc_show(t,db,logs); break;

		case T_SITE_LIST:
			task_site_list(t,db,logs); break;
		case T_SITE_SHOW:
			task_site_show(t,db,logs); break;
		case T_SITE_ADD:
			task_site_add(t,sites,db,logs); printf("paso case\n"); break;
		case T_SITE_DEL:
			task_site_del(t,sites,db,logs); break;
		case T_SITE_MOD:
			task_site_mod(t,sites,db,logs); break;
		case T_SITE_STOP:
			task_site_stop(t,sites,db,logs); break;
		case T_SITE_START:
			task_site_start(t,sites,db,logs); break;

		case T_SERVER_LIST:
			task_server_list(t,db,workers,proxys); break;
		case T_SERVER_SHOW:
			task_server_show(t,db,workers,proxys); break;
		case T_SERVER_STOP:
			task_server_stop(t,db,workers,proxys); break;
		case T_SERVER_START:
			task_server_start(t,db,workers,proxys); break;
		default:
			printf("ERROR FATAL. TASK_TYPE indefinido\n");

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

int heap_task_exist(T_heap_task *h, T_taskid id){
	/* indica si el trabajo existe en la cola */
	heap_t_node *aux;
	int exist=0;
	
	aux = h->first;
	while(!exist && aux!= NULL){
		exist = (strcmp(task_get_id(aux->data),id) == 0);
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

