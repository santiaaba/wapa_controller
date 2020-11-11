#include "structs.h"

/*****************************
	 Varios 
******************************/

void itosstatus(T_site_status i, char *name){
	switch(i){
		case S_ONLINE: strcpy(name,"ONLINE"); break;
		case S_OFFLINE: strcpy(name,"OFFLINE"); break;
	}
}

void itoscstatus(T_sc_status i, char *name){
	switch(i){
		case SC_OK: strcpy(name,"OK"); break;
		case SC_POOR: strcpy(name,"POOR"); break;
		case SC_NONE: strcpy(name,"NONE"); break;
	}
}

void itowstatus(T_worker_status i, char *name){
	switch(i){
		case W_ONLINE: strcpy(name,"ONLINE"); break;
		case W_UNKNOWN: strcpy(name,"UNKNOWN"); break;
		case W_OFFLINE: strcpy(name,"OFFLINE"); break;
		case W_BROKEN: strcpy(name,"BROKEN"); break;
		case W_PREPARED: strcpy(name,"PREPARED"); break;
	}
}

void itopstatus(T_proxy_status i, char *name){
	switch(i){
		case P_ONLINE: strcpy(name,"ONLINE"); break;
		case P_UNKNOWN: strcpy(name,"UNKNOWN"); break;
		case P_OFFLINE: strcpy(name,"OFFLINE"); break;
		case P_BROKEN: strcpy(name,"BROKEN"); break;
		case P_PREPARED: strcpy(name,"PREPARED"); break;
	}
}

void int_to_4bytes(uint32_t *i, char *_4bytes){
    memcpy(_4bytes,i,4);
}

void _4bytes_to_int(char *_4bytes, uint32_t *i){
    memcpy(i,_4bytes,4);
}

/*****************************
	 Sitios
******************************/
void site_init(T_site *s, char *name, uint32_t id, char *dir,
	       unsigned int version, unsigned int size){

	s->workers = (T_lista*)malloc(sizeof(T_lista));
	s->alias = (T_lista*)malloc(sizeof(T_lista));
	s->indexes = (T_lista*)malloc(sizeof(T_lista));
	s->dir = NULL;

	lista_init(s->workers,sizeof(T_worker));
	lista_init(s->alias,sizeof(T_s_e));
	lista_init(s->indexes,sizeof(T_s_e));
	strcpy(s->name,name);
	dim_copy(&(s->dir),dir);
	s->status = W_ONLINE;
	s->sc_status = SC_NONE;
	s->id = id;
	s->version = version;
	s->size = size;
}

T_lista *site_get_workers(T_site *s){
	return s->workers;
}

uint32_t site_get_id(T_site *s){
	return s->id;
}

unsigned int site_get_version(T_site *s){
	return s->version;
}

char *site_get_dir(T_site *s){
	return s->dir;
}

char *site_get_name(T_site *s){
	return s->name;
}

unsigned int site_get_size(T_site *s){
	return s->size;
}

unsigned int site_get_real_size(T_site *s){
	return lista_size(s->workers);
}

T_lista *site_get_alias(T_site *s){
	return s->alias;
}

T_lista *site_get_indexes(T_site *s){
	return s->indexes;
}

T_site_status site_get_status(T_site *s){
	return s->status;
}

void site_set_size(T_site *s, unsigned int size){
	if(size < SITE_MAX_SIZE){
		s->size = size;
	}
}

void site_set_status(T_site *s, T_site_status status){
	s->status = status;
}

void site_update(T_site *s){
	/* Le indica a los workers que deben actualziar la iformacion
	 * del sitio */
	T_worker *worker;

	lista_first(site_get_workers(s));
	while(!lista_eol(site_get_workers(s))){
		worker = lista_get(site_get_workers(s));
		worker_add_site(worker,s);
		lista_next(site_get_workers(s));
	}
}

void site_stop(T_site *s){
	s->status = S_OFFLINE;
	site_update(s);
}

void site_start(T_site *s){
	s->status = S_ONLINE;
	site_update(s);
}

void site_to_json(T_site *s,char **message){
	char aux[200];
	char *ws = NULL;
	char *ptr;
	dim_init(message);
	dim_copy(message,"{\"name\":\"");
	dim_concat(message,s->name);
	dim_concat(message,"\",\"id\":\"");
	sprintf(aux,"%llu", s->id);
	dim_concat(message,aux);
	dim_concat(message,"\",\"directory\":\"");
	dim_concat(message,s->dir);
	dim_concat(message,"\",\"version\":\"");
	sprintf(aux,"%lu", s->version);
	dim_concat(message,aux);
	dim_concat(message,"\",\"status\":\"");
	itosstatus(s->status,aux);
	dim_concat(message,aux);
	dim_concat(message,"\",\"clusterStatus\":\"");
	itoscstatus(s->sc_status,aux);
	dim_concat(message,aux);
	dim_concat(message,"\",\"workers\":");
	lista_to_json(s->workers,&ws,worker_to_json);
	dim_concat(message,ws);
	dim_concat(message,"}");
}
/*****************************
	 Workers
******************************/
void worker_init(T_worker *w, int id, char *name, char *ip, T_worker_status s){
	printf("worker_init entro\n");
	strcpy(w->name,name);
	strcpy(w->ip,ip);
	w->id = id;
	w->sites = (T_lista*)malloc(sizeof(T_lista));
	lista_init(w->sites,sizeof(T_site));
	w->laverage = 0.0;
	w->server.sin_addr.s_addr = inet_addr(ip);
	w->server.sin_family = AF_INET;
	w->server.sin_port = htons(3550);
	w->status = s;
	w->last_status = s;
	w->time_change_status = time(0);
	w->is_changed = 0;
	worker_connect(w);
	printf("worker_init salio\n");
}
int worker_connect(T_worker *w){
	/* Funcion de uso interno */
	/* Intenta conectarse al worker */

	close(w->socket);
	w->socket = socket(AF_INET , SOCK_STREAM , 0);
	printf("Intentando conectar worker\n");
	if (connect_wait(w->socket , (struct sockaddr *) &(w->server) , sizeof(w->server),5) != 0){
	//if (connect(w->socket , (struct sockaddr *) &(w->server) , sizeof(w->server)) < 0){
		printf("Worker %s NO CONECTA\n",w->name);
		w->socket=0;
		return 0;
	}
	printf("Worker %s CONECTO\n",w->name);
	return 1;
}

int worker_end_connect(T_worker *w){
	/* Funcion de uso interno */
	/* Finaliza la coneccion contra el worker fisico */
	close(w->socket);
}

unsigned int worker_get_last_time(T_worker *w){
	return w->time_change_status;
}

void worker_change_status(T_worker *w, T_worker_status s){
	/* funcion de uso interno */
	/* Retorna 1 si cambio el estado. 0 en caso contrario */
	char aux[100];
	char aux2[100];
	
	/* Solo actualizamos el estado si es distinto
 	 * del que ya posee */
	itowstatus(w->last_status,aux);
	itowstatus(s,aux2);
	if(w->status != s){
		printf("change_status: %s -> %s\n",aux,aux2);
		w->time_change_status = (unsigned long)time(0);
		w->last_status = w->status;
		w->status = s;
		w->is_changed = 1;
	}
}

T_worker_status worker_get_status(T_worker *w){
	return w->status;
}

T_worker_status worker_get_last_status(T_worker *w){
	return w->last_status;
}

int worker_get_id(T_worker *w){
	return w->id;
}

void worker_update_state(T_worker *w){
	/* Actualiza el estado del worker bajo demanda */
	w->status;
}

float worker_num_sites(T_worker *w){
	return (float)lista_size(w->sites);
}

void worker_start(T_worker *w){
	worker_change_status(w,W_PREPARED);
}

void worker_stop(T_worker *w){
	worker_change_status(w,W_OFFLINE);
}

char *worker_get_name(T_worker *w){
	return w->name;
}

char *worker_get_ipv4(T_worker *w){
	return w->ip;
}

float worker_get_load(T_worker *w){
	return w->laverage;
	//return 0.0;
}

T_lista *worker_get_sites(T_worker *w){
	return w->sites;
}

void worker_purge(T_worker *w){
	/* Se desasignan todos los sitios del worker.
	 * Se eliminan tambien los archivos fisicos */

	T_site *site;
	char send_message[2];
	char *rcv_message = NULL;
	uint32_t rcv_message_size=0;

	sprintf(send_message,"P");
	printf("PURGE: Eliminamos sitios fisicos\n");
	if(worker_send_receive(w,send_message,(uint32_t)strlen(send_message)+1,&rcv_message,&rcv_message_size)){
		lista_first(w->sites);
		printf("PURGE: Eliminamos sitios logicos\n");
		printf("PURGE: Entrando while\n");
		printf("El worker %s posee %i sitios\n",worker_get_name(w),lista_size(w->sites));
		while(!lista_eol(w->sites)){
			printf("Obteniendo sitio\n");
			site = lista_get(w->sites);
			printf("PURGE: Removiendo worker del sitio\n");
			lista_exclude(site_get_workers(site),worker_get_id,worker_get_id(w));
			printf("PURGE: proximo sitio\n");
			lista_next(w->sites);
		}
		printf("PURGE: borrando lista de sitios del worker\n");
		lista_erase(w->sites);
	}
	free(rcv_message);
}

void worker_set_statistics(T_worker *w, char *buffer_rx){
	/* Funcion interna. Actualiza las estadisticas
 	 * en base a lo que recibe del worker fisico */
	char aux[10];
	int pos=2;
	parce_data(buffer_rx,'|',&pos,aux); // Obtiene el mensaje
	parce_data(buffer_rx,'|',&pos,aux); // average 1 min
	parce_data(buffer_rx,'|',&pos,aux); // average 5 min
	parce_data(buffer_rx,'|',&pos,aux); // average 15 min
	w->laverage = atof(aux);
}

int worker_check(T_worker *w){
	/* Verifica un worker. Actualiza el estado del mismo */
	/* Retorna 1 si cambio de estado. 0 En caso contrario */

	char send_message[2];
	char *rcv_message = NULL;
	uint32_t rcv_message_size;

	/* Verificamos si responde correctamente */
	/* Si esta OFFLINE no hacemos nada. Sigue en OFFLINE */
	
	printf("Tiempo entre estados %lu - %lu = %lu\n",(unsigned long)time(0),(unsigned long)w->time_change_status,(unsigned long)time(0) - (unsigned long)w->time_change_status);
	if(w->status != W_OFFLINE){
		sprintf(send_message,"C");
		if(worker_send_receive(w,send_message,(uint32_t)strlen(send_message)+1,&rcv_message,&rcv_message_size)){
			printf("CHECK: -%s-\n",rcv_message);
			/* Actualizamos las estadisticas */
			worker_set_statistics(w, rcv_message);
			if(rcv_message[0] == '1'){
				if(w->status == W_BROKEN || w->status == W_UNKNOWN){
					/* Si estaba en BROKEN o UNKNOWN pasa a PREPARED */
					worker_change_status(w,W_PREPARED);
				} else {
					if((w->status == W_PREPARED) &&
					   ((unsigned long)time(0) - w->time_change_status) > TIMEONLINE){
						 /* Responde, pasa el chequeo, 
						  * ya estaba en PREPARED y paso el tiempo */
						 worker_change_status(w,W_ONLINE);
					}
				}
			} else {
				printf("worker %s ROTO!\n",worker_get_name(w));
				worker_change_status(w,W_BROKEN);
			}
		} else {
			printf("worker %s NO RESPONDE!\n",worker_get_name(w));
		}
	}
	free(rcv_message);
	if (w->is_changed){
		w->is_changed = 0;
		return 1;
	} else {
		return 0;
	}
}

int worker_add_site(T_worker *w, T_site *s){
	/* Agrega fisica y logicamente un sitio a un worker.
	 * Si no pudo hacerlo retorna 0 caso contrario 1 */

	char aux[100];
	char *send_message = NULL;
	uint32_t send_message_size;
	char *rcv_message = NULL;
	uint32_t rcv_message_size;
	int ok;

	T_s_e *aux_s_e;

	printf("WORKER_ADD_SITE: Entro\n");
	send_message_size = 100;
	send_message = (char *)malloc(send_message_size);
	sprintf(send_message,"A%lu|%s|%s|%i|%i|",site_get_id(s),
		site_get_name(s),site_get_dir(s),site_get_version(s),
		site_get_status(s));
	/* Esta fallando el sprintf con 5 variables */
	printf("SITIO DE MOMENTO: %s\n",send_message);

	// Armar los alias
	printf("WORKER_ADD_SITE: armamos alias\n");
	lista_first(site_get_alias(s));
	ok=0;
	while(!lista_eol(site_get_alias(s))){
		ok = 1;
		aux_s_e = lista_get(site_get_alias(s));
		printf("alias agregado: %s\n",s_e_get_name(aux_s_e));
		sprintf(aux,"%s,",s_e_get_name(aux_s_e));
		if(send_message_size < strlen(send_message) + strlen(aux) + 2){
			printf("allocando espacio\n");
			send_message_size += 100;
			send_message = (char *)realloc(send_message,send_message_size);
		}
		strcat(send_message,aux);
		lista_next(site_get_alias(s));
	}
	if(ok)
		send_message[strlen(send_message)-1] = '|';
	else
		strcat(send_message,"|");

	// Armar los indices
	printf("WORKER_ADD_SITE: armamos indices\n");
	lista_first(site_get_indexes(s));
	ok=0;
	while(!lista_eol(site_get_indexes(s))){
		ok=1;
		aux_s_e = lista_get(site_get_indexes(s));
		printf("index agregado: %s\n",s_e_get_name(aux_s_e));
		sprintf(aux,"%s,",s_e_get_name(aux_s_e));
		if(send_message_size < strlen(send_message) + strlen(aux) + 2){
			send_message_size += 100;
			send_message = (char *)realloc(send_message,send_message_size);
		}
		strcat(send_message,aux);
		lista_next(site_get_indexes(s));
	}
	if(ok)
		send_message[strlen(send_message)-1] = '\0';

	/* Normalizamos el send_message */
	send_message_size = strlen(send_message)+1;
	send_message = (char *)realloc(send_message,send_message_size);
	
	printf("WORKER_ADd_SITE: enviamos mensaje\n");
	ok=1;
	if(worker_send_receive(w,send_message,send_message_size,&rcv_message,&rcv_message_size)){
		if(rcv_message[0] == '1'){
			lista_add(w->sites,s);
			lista_add(site_get_workers(s),w);
		}  else {
			ok=0;
		}
	} else {
		ok=0;
	}
	free(send_message);
	free(rcv_message);
	return ok;
}

int worker_remove_site(T_worker *w, T_site *s){
	/* Remueve fisica y logicamente un sitio de un worker */
	char send_message[100];
	char *rcv_message=NULL;
	uint32_t rcv_message_size;
	int ok=1;

	sprintf(send_message,"d%s",site_get_name(s));

	if(worker_send_receive(w,send_message,(uint32_t)strlen(send_message)+1,&rcv_message,&rcv_message_size)){
		lista_exclude(worker_get_sites(w),site_get_id,site_get_id(s));
		lista_exclude(site_get_workers(s),worker_get_id,worker_get_id(w));
	} else {
		ok=0;
	}
	free(rcv_message);
	return ok;
}

int worker_send_receive(T_worker *w, char *send_message, uint32_t send_message_size,
			char **rcv_message, uint32_t *rcv_message_size){
	/* Se conecta al worker, envia un comando y espera la respuesta */
	char buffer[ROLE_BUFFER_SIZE];
	char printB[ROLE_BUFFER_SIZE+1];
	uint32_t parce_size;
	int first_message=1;
	int pos;
	uint32_t c=0;

	/* Verificamos que el final del send_message sea '\0' */
	if( send_message[send_message_size-1] != '\0'){
		printf("worker_send_receive: ERROR. send_message no termina en \\0 = %c\n",send_message[send_message_size]);
		return 0;
	}
	
	printf("WORKER SEND: send_message_size=:%u, send_message=%s\n",send_message_size,send_message);
	/* Los 4 primeros bytes del header es el tamano total del mensaje */
	int_to_4bytes(&send_message_size,buffer);

	//printf("paso 1: %s - %i\n",send_message,send_message_size);
	while(c < send_message_size){
		/* Hay que incluir un header de tamano ROLE_HEADER_SIZE */
		if(send_message_size - c + ROLE_HEADER_SIZE < ROLE_BUFFER_SIZE){
			/* Entra todo en el buffer */
			parce_size = send_message_size - c ;
		} else {
			/* No entra todo en el buffer */
			parce_size = ROLE_BUFFER_SIZE - ROLE_HEADER_SIZE;
		}
		//printf("paso 1.1 parce_size: %i\n",parce_size);
		int_to_4bytes(&parce_size,&(buffer[4]));
		//printf("paso 1.2\n");
		memcpy(buffer + ROLE_HEADER_SIZE,send_message + c,parce_size);
		//printf("paso 1.3\n");
		c += parce_size;
		printf("Enviando:'%s' size-fijo:%i\n", buffer,ROLE_BUFFER_SIZE);
		if(send(w->socket,buffer,ROLE_BUFFER_SIZE,0)<0){
			worker_change_status(w,W_UNKNOWN);
			worker_connect(w);
			return 0;
		}
		//printf("paso 1.4\n");
	}
	//printf("paso 2\n");

	/* Recibir */
	c=0;
	/* Al menos una recepcion esperamos recibir */
	int_to_4bytes(&c,buffer);
	int_to_4bytes(&c,&(buffer[4]));
	//printf("paso 3:\n");
	do{
		 if(recv(w->socket,buffer,ROLE_BUFFER_SIZE,0)<0){
			worker_change_status(w,W_UNKNOWN);
			worker_connect(w);
			return 0;
		}
		/* Del header obtenemos el tamano de los datos que
 		 * recibiremos */
		if(first_message){
			first_message=0;
			_4bytes_to_int(buffer,rcv_message_size);
			printf("WORKER RCV: Nos enviaran: %u bytes\n",*rcv_message_size);
			*rcv_message=(char *)realloc(*rcv_message,*rcv_message_size);
		}

		_4bytes_to_int(&(buffer[4]),&parce_size);
		memcpy(*rcv_message+c,&(buffer[ROLE_HEADER_SIZE]),parce_size);
		c += parce_size;
	} while (c < *rcv_message_size);
	printf("WORKER RCV: rcv_message_size=:%u, rcv_message=%s\n",*rcv_message_size,*rcv_message);
	return 1;
}

int worker_sync(T_worker *w, T_lista *s){
	/* Se conecta al worker, obtiene el listado
	 * de sitios y actualiza las estructuras */

	char send_message[2];
	char *rcv_message=NULL;
	uint32_t rcv_message_size=0;
	int pos = 0;
	char aux[10];
	T_site *site;
	int ok=1;

	printf("-- Entramos a Sync %s\n",w->name);

	sprintf(send_message,"G");

	if(!worker_send_receive(w,send_message,(uint32_t)strlen(send_message)+1,&rcv_message,&rcv_message_size)){
		ok=0;
	}
	printf("Mensaje obtenido worker: %s\n",rcv_message);
	while(pos<rcv_message_size){
		parce_data(rcv_message,'|',&pos,aux);
		site = lista_find(s,site_get_id,atoi(aux));
		if(site){
			lista_add(w->sites,site);
			lista_add(site_get_workers(site),w);
		} else {
			/* Ese sitio ya no existe en el sistema. lo eliminamos del worker */
			printf ("IMPLEMENTER ELIMINACION DEL WORKER SITIOS YA NO EXISTENTES\n");
		}
	}
	printf("-- Terminamos SYNC %s\n",w->name);
	free(rcv_message);
	return ok;
}

int worker_reload(T_worker *w){
	/* Le indica a un proxy que recargue su configuracion */
	char send_message[2];
	char *rcv_message=NULL;
	uint32_t rcv_message_size=0;
	int ok;

	sprintf(send_message,"R");
	ok = worker_send_receive(w,send_message,(uint32_t)strlen(send_message)+1,&rcv_message,&rcv_message_size);
	free(rcv_message);
	return ok;
}

void worker_to_json(T_worker *w,char **message){
	char aux[200];
	char *ptr;
	dim_init(message);
	dim_copy(message,"{\"name\":\"");
	dim_concat(message,w->name);
	dim_concat(message,"\"}");
}


/*****************************
	 Proxys
******************************/
void proxy_init(T_proxy *p, int id, char *name, char *ip, T_proxy_status s){
	strcpy(p->name,name);
	strcpy(p->ip,ip);
	p->id = id;
	p->laverage = 0.0;
	p->server.sin_addr.s_addr = inet_addr(ip);
	p->server.sin_family = AF_INET;
	p->server.sin_port = htons(3550);
	p->status = s;
	p->last_status = s;
	p->time_change_status = time(0);
	p->is_changed = 0;
	proxy_connect(p);
}

int proxy_connect(T_proxy *p){
	/* Funcion de uso interno */
	/* Intenta conectarse al proxy */

	close(p->socket);
	p->socket = socket(AF_INET , SOCK_STREAM , 0);
	if (connect_wait(p->socket , (struct sockaddr *) &(p->server) , sizeof(p->server),5) != 0){
		printf("Proxy %s NO CONECTA\n",p->name);
		return 0;
	}
	printf("Proxy %s CONECTO\n",p->name);
	return 1;
}

unsigned int proxy_get_last_time(T_proxy *p){
	return p->time_change_status;
}

void proxy_change_status(T_proxy *p, T_proxy_status s){
	/* Solo actualizamos el estado si es distinto
	 * del que ya posee */
	if(p->status != s){
		p->time_change_status = (unsigned long)time(0);
		p->last_status = p->status;
		p->status = s;
		p->is_changed = 1;
	}
}

T_proxy_status proxy_get_status(T_proxy *p){
	return p->status;
}

T_proxy_status proxy_get_last_status(T_proxy *p){
	return p->last_status;
}

int proxy_get_id(T_proxy *p){
	return p->id;
}

void proxy_start(T_proxy *p){
	proxy_change_status(p,P_PREPARED);
}

void proxy_stop(T_proxy *p){
	proxy_change_status(p,P_OFFLINE);
}

char *proxy_get_name(T_proxy *p){
	return p->name;
}
char *proxy_get_ipv4(T_proxy *p){
	return p->ip;
}

float proxy_get_load(T_proxy *p){
	return p->laverage;
}

void proxy_set_statistics(T_proxy *p, char *buffer_rx){
	/* Funcion interna. Actualiza las estadisticas
	 * en base a lo que recibe del proxy fisico */
	char aux[10];
	int pos=2;
	parce_data(buffer_rx,'|',&pos,aux); // Obtiene el mensaje
	parce_data(buffer_rx,'|',&pos,aux); // average 1 min
	parce_data(buffer_rx,'|',&pos,aux); // average 5 min
	parce_data(buffer_rx,'|',&pos,aux); // average 15 min
	p->laverage = atof(aux);
}

void proxy_reconfig(T_proxy *p, T_lista *sites){
	/* Reconfigura un proxy en base a la informacion
	   de los sitios. Borra toda configuracion anterior.
	   De momento un proxy ve todos los sitios y no unos en particular */

	char send_message[200];
	char *rcv_message=NULL;
	uint32_t rcv_message_size=0;

	sprintf(send_message,"D");
	if(proxy_send_receive(p,send_message,2,&rcv_message,&rcv_message_size)){
		lista_first(sites);
		while(!lista_eol(sites)){
			printf("Agregamos el sitio al proxy\n");
			proxy_add_site(p,lista_get(sites));
			lista_next(sites);
			printf("Sitio siguiente\n");
		}
	}
}

int proxy_add_site(T_proxy *p, T_site *s){
	/* Reconfigura el proxy para el sitio indicado */

	char *send_message=NULL;
        char *rcv_message=NULL;
        uint32_t rcv_message_size=0;
        uint32_t send_message_size=0;
	T_s_e *aux_s_e;
	T_worker *worker;
	char aux[512];
	int ok=1;

	/* Si el sitio no tiene workers entonces mas
 	 * que agregarlo lo eliminamos */

	send_message=(char *)malloc(100);
	if(lista_size(site_get_workers(s)) == 0){
		sprintf(send_message,"d%s",site_get_name(s));
		send_message_size=strlen(send_message)+1;
		if(!proxy_send_receive(p,send_message,send_message_size,&rcv_message,&rcv_message_size))
			ok = 0;
		ok = 1;
	} else {
	/* Si posee workers entonces proseguimos */
		
		sprintf(send_message,"A%s|%lu|%i|",site_get_name(s),
			site_get_id(s),site_get_version(s));

		printf("Armamos los workers\n");
		lista_first(site_get_workers(s));
		while(!lista_eol(site_get_workers(s))){
			worker = lista_get(site_get_workers(s));
			sprintf(aux,"%s,",worker_get_name(worker));
			if(send_message_size < strlen(send_message) + strlen(aux) + 2){
				printf("allocando espacio\n");
				send_message_size += 100;
				send_message = (char *)realloc(send_message,send_message_size);
			}
			strcat(send_message,aux);
			lista_next(site_get_workers(s));
		}
		send_message[strlen(send_message)-1] = '|';
		
		printf("Armamos los alias\n");
		lista_first(site_get_alias(s));
		while(!lista_eol(site_get_alias(s))){
			aux_s_e = lista_get(site_get_alias(s));
			printf("alias agregado: %s\n",s_e_get_name(aux_s_e));
			sprintf(aux,"%s,",s_e_get_name(aux_s_e));
			if(send_message_size < strlen(send_message) + strlen(aux) + 2){
				printf("allocando espacio\n");
				send_message_size += 100;
				send_message = (char *)realloc(send_message,send_message_size);
			}
			strcat(send_message,aux);
			lista_next(site_get_alias(s));
		}
		/* Normalizamos el send_message */
		send_message_size = strlen(send_message)+1;
		send_message = (char *)realloc(send_message,send_message_size);
	
		printf("PROXY_ADD_SITE: enviamos mensaje\n");
		if(proxy_send_receive(p,send_message,send_message_size,&rcv_message,&rcv_message_size)){
			ok=1;
		} else {
			ok=0;
		}
	}
	free(rcv_message);
	free(send_message);
	return ok;
}

int proxy_change_site(T_proxy *p, T_site *s){

	char send_message[200];
        char *rcv_message=NULL;
        uint32_t rcv_message_size=0;

	/* Eliminamos la configuracion anterior */
	sprintf(send_message,"d%s",site_get_name(s));
	if(!proxy_send_receive(p,send_message,strlen(send_message)+1,&rcv_message,&rcv_message_size))
		return 0;
	/* Enviamos la configuracion nueva */
	printf("Enviamos configuracion\n");
	return proxy_add_site(p,s);
}

int proxy_reload(T_proxy *p){
	/* Le indica a un proxy que recargue su configuracion */

	char *rcv_message=NULL;
        uint32_t rcv_message_size=0;

	return(proxy_send_receive(p,"R\0",2,&rcv_message,&rcv_message_size));
}

int proxy_check(T_proxy *p){
	/* Verifica un proxy. Actualiza el estado del mismo */
	/* Retorna 1 si cambio de estado. 0 En caso contrario */

	char *rcv_message=NULL;
        uint32_t rcv_message_size=0;

	/* Verificamos si responde correctamente */
	/* Si esta OFFLINE no hacemos nada. Sigue en OFFLINE */

	printf("Tiempo entre estados %lu - %lu = %lu\n",(unsigned long)time(0),
		(unsigned long)p->time_change_status,
		(unsigned long)time(0) - (unsigned long)p->time_change_status);
	if(p->status != P_OFFLINE){
		if(proxy_send_receive(p,"C\0",2,&rcv_message,&rcv_message_size)){
			printf("CHECK: -%s-\n",rcv_message);
			/* Actualizamos las estadisticas */
			proxy_set_statistics(p, rcv_message);
			if(rcv_message[0] == '1'){
				if(p->status == P_BROKEN || p->status == P_UNKNOWN){
					/* Si estaba en BROKEN o UNKNOWN pasa a PREPARED */
					proxy_change_status(p,P_PREPARED);
				} else {
					if((p->status == P_PREPARED) &&
					   ((unsigned long)time(0) - p->time_change_status) > TIMEONLINE){
						 /* Responde, pasa el chequeo,
						  * ya estaba en PREPARED y paso el tiempo */
						 proxy_change_status(p,P_ONLINE);
					}
				}
			} else {
				printf("proxy %s ROTO!\n",proxy_get_name(p));
				proxy_change_status(p,P_BROKEN);
			}
		} else {
			printf("proxy %s NO RESPONDE!\n",proxy_get_name(p));
		}
	}
	if (p->is_changed){
		p->is_changed = 0;
		return 1;
	} else {
		return 0;
	}
}

int proxy_send_receive(T_proxy *p, char *send_message, uint32_t send_message_size,
                        char **rcv_message, uint32_t *rcv_message_size){
	/* Se conecta al proxy, envia un comando y espera la respuesta */

	char buffer[ROLE_BUFFER_SIZE];
	char printB[ROLE_BUFFER_SIZE+1];
	uint32_t parce_size;
	int first_message=1;
	int pos;
	uint32_t c=0;

	/* Verificamos que el final del send_message sea '\0' */
	if( send_message[send_message_size-1] != '\0'){
		printf("proxy_send_receive: ERROR. send_message no termina en \\0 = %c\n",send_message[send_message_size]);
		return 0;
	}
	
	printf("SEND-------SEND----SEND-----\n");
	printf("Mensaje al proxy. ROLE_BUFFER_SIZE=%i , send_message_size=:%i, send_message=%s\n",ROLE_BUFFER_SIZE,send_message_size,send_message);
	/* Los 4 primeros bytes del header es el tamano total del mensaje */
	int_to_4bytes(&send_message_size,buffer);

	while(c < send_message_size){
		/* Hay que incluir un header de tamano ROLE_HEADER_SIZE */
		if(send_message_size - c + ROLE_HEADER_SIZE < ROLE_BUFFER_SIZE){
			/* Entra todo en el buffer */
			parce_size = send_message_size - c ;
		} else {
			/* No entra todo en el buffer */
			parce_size = ROLE_BUFFER_SIZE - ROLE_HEADER_SIZE;
		}
		int_to_4bytes(&parce_size,&(buffer[4]));
		memcpy(buffer + ROLE_HEADER_SIZE,send_message + c,parce_size);
		c += parce_size;
		if(send(p->socket,buffer,ROLE_BUFFER_SIZE,0)<0){
			proxy_change_status(p,P_UNKNOWN);
			proxy_connect(p);
			return 0;
		}
	}

	/* Recibir */
	c=0;
	/* Al menos una recepcion esperamos recibir */
	printf("RECEIV-------RECEIV-------RECEIV-----\n");
	int_to_4bytes(&c,buffer);
	int_to_4bytes(&c,&(buffer[4]));
	do{
		 if(recv(p->socket,buffer,ROLE_BUFFER_SIZE,0)<0){
			proxy_change_status(p,P_UNKNOWN);
			proxy_connect(p);
			return 0;
		}
		/* Del header obtenemos el tamano de los datos que
 		 * recibiremos */
		if(first_message){
			first_message=0;
			_4bytes_to_int(buffer,rcv_message_size);
			*rcv_message=(char *)realloc(*rcv_message,*rcv_message_size);
		}

		_4bytes_to_int(&(buffer[4]),&parce_size);
		memcpy(*rcv_message+c,&(buffer[ROLE_HEADER_SIZE]),parce_size);
		c += parce_size;
	} while (c < *rcv_message_size);
	printf("RECEIV completo: %s\n",*rcv_message);
	return 1;

}

/*****************************
	   string Element
 ******************************/
void s_e_init(T_s_e *a, unsigned int id, char *name){
	a->id = id;
	a->name = (char *)malloc(strlen(name)+1);
	strcpy(a->name,name);
	printf("Nombre copiado %s = %s\n",name,a->name);
}

char *s_e_get_name(T_s_e *a){
	return a->name;
}

int s_e_get_id(T_s_e *a){
	return a->id;
}

void s_e_free(T_s_e **a){
	free((*a)->name);
	free(*a);
}
