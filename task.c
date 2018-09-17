#include "task.h"

void random_task_id(T_taskid value){
	/*Genera un string random para task_id */
	char *string = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	int i,j;
	
	for(j=0;j<TASKID_SIZE-1;j++){
		i = rand() % 62;
		//printf("i=%i\n",i);
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

/*****************************
	     TASK 
******************************/
void task_init(T_task *t, T_task_type type, T_dictionary *data){
	random_task_id(t->id);
	random_token(t->token);
	t->type = type;
	t->data = data;
	t->result = (char *)malloc(TASKRESULT_SIZE);
	t->result_size = TASKRESULT_SIZE;
	strcpy(t->result,"");
}

void task_destroy(T_task **t){
	printf("Entramos a eliminar el task\n");
	if((*t)->data != NULL){
		printf("Eliminamos el diccionario\n");
		dictionary_destroy(&((*t)->data));
		//free((*t)->data);
	}
	printf("liberamos el resultado\n");
	free((*t)->result);
	printf("liberamos el task\n");
	free(*t);
}

void task_site_list(T_task *t, T_list_site *l){
	json_sites(&(t->result),&(t->result_size),l);
}

void task_site_show(T_task *t, T_list_site *l){
	char *id;
	T_site *site;

	id = dictionary_get(t->data,"id");
	site = list_site_find_id(l,atoi(id));
	json_site(&(t->result),&(t->result_size),site);
}

char *task_get_result(T_task *t){
	return t->result;
}

char *task_get_id(T_task *t){
	return t->id;
}

void task_worker_list(T_task *t, T_list_worker *l){
	json_workers(&(t->result),&(t->result_size),l);
}

void task_worker_show(T_task *t, T_list_worker *l){
	char *id;
	T_worker *worker;

	id = dictionary_get(t->data,"id");
	worker = list_worker_find_id(l,atoi(id));
	json_worker(&(t->result),&(t->result_size),worker);
}

char *task_get_token(T_task *t){
	return t->token;
}

char *tob_get_id(T_task *t){
	return t->id;
}

int task_site_add(T_task *t, T_list_site *l, T_db *db){
	/* Agrega un sitio a la solucion */

	T_site *newsite;
	char sql[300];
	char *name;
	char *susc_id;
	char command[300];
	char dir[10];
	unsigned int id;
	
	printf("Agregar un sitio nuevo\n");
	dictionary_print(t->data);
	name = dictionary_get(t->data,"name");
	susc_id = dictionary_get(t->data,"susc_id");

	/* Los tres valores anteriores no pueden ser vacio */
	if(strcmp(susc_id,"") == 0){ return 0; }
	if(strcmp(name,"") == 0){ return 0; }

	/* Verificamos que el sitio no exista ya con ese nombre */
	printf("Verificamos si el sitio existe\n");
	if(db_find_site(db,name)){ printf("NOOO\n"); return 0; }

	// Alta en la base de datos del sitio
	printf("Alta a la base de datos\n");
	if(!db_add_site(db,&newsite,name,atoi(susc_id),dir)){ return 0; }

	// Creacion del espacio de almacenamiento
	printf("Creacion de directorios\n");
	sprintf(command,"mkdir -p /websites/%s/%s/wwwroot",dir,name);
	if(system(command) != 0){ return 0; }
	sprintf(command,"mkdir -p /websites/%s/%s/logs",dir,name);
	if(system(command) != 0){ return 0; }

	// Adicion del sitio a la lista
	printf("agregado al listado\n");
	list_site_add(l,newsite);

	return 1;
}

int task_site_del(T_task *t, T_list_site *l, T_db *db){
}

int task_site_mod(T_task *t, T_list_site *l, T_db *db){
}

int task_worker_stop(T_task *t, T_list_worker *l, T_db *db){
	/* Detenemos el worker y lo indicamos en
 	 * la base de datos */
	int id = atoi(dictionary_get(t->data,"id"));
	worker_stop(list_worker_find_id(l,id));
	//db_worker_stop(db,id);
}

int task_worker_start(T_task *t, T_list_worker *l, T_db *db){
	/* Arrancamos el worker y lo indicamos en
 	 * la base de datos */
	int id = atoi(dictionary_get(t->data,"id"));
	worker_start(list_worker_find_id(l,id));
	//db_worker_start(db,id);
}

int task_site_stop(T_task *t, T_list_site *l, T_db *db){
}

int task_site_start(T_task *t, T_list_site *l, T_db *db){
}

T_task_type task_c_to_type(char c){
	switch(c){
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

void task_run(T_task *t, T_list_site *sites, T_list_worker *workers,
		T_list_proxy *proxys, T_db *db){
	/* Ejecuta el JOB */
	printf("paso\n");
	t->status = T_RUNNING;
	printf("paso\n");

	switch(t->type){
		case T_SITE_LIST:
			task_site_list(t,sites); break;
		case T_SITE_SHOW:
			task_site_show(t,sites); break;
		case T_SITE_ADD:
			task_site_add(t,sites,db); break;
		case T_SITE_DEL:
			task_site_del(t,sites,db); break;
		case T_SITE_MOD:
			task_site_mod(t,sites,db); break;
		case T_SITE_STOP:
			task_site_stop(t,sites,db); break;
		case T_SITE_START:
			task_site_start(t,sites,db); break;

//		case T_SERVER_LIST:
//			task_server_list(t,workers); break;
//		case T_SERVER_SHOW:
//			task_server_show(t,workers); break;
//		case T_SERVER_STOP:
//			task_server_stop(t,workers,db); break;
//		case T_SERVER_START:
//			task_server_start(t,workers,db); break;
	}
	t->status = T_DONE;
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

	printf("Entroooo\n");
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
			b->actual = b->actual->next;
		}
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

