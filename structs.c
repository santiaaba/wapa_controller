#include "structs.h"

/*****************************
	 Varios 
******************************/

void itowstatus(T_worker_status i, char *name){
	switch(i){
		case W_ONLINE: strcpy(name,"W_ONLINE"); break;
		case W_UNKNOWN: strcpy(name,"W_UNKNOWN"); break;
		case W_OFFLINE: strcpy(name,"W_OFFLINE"); break;
		case W_BROKEN: strcpy(name,"W_BROKEN"); break;
		case W_PREPARED: strcpy(name,"W_PREPARED"); break;
	}
}

/*****************************
	 Sitios
******************************/
void site_init(T_site *s, char *name, unsigned int id, char *dir,
	       unsigned int version, unsigned int size){
	s->workers = (T_list_worker*)malloc(sizeof(T_list_worker));
	s->alias = (T_list_s_e*)malloc(sizeof(T_list_s_e));
	s->indexes = (T_list_s_e*)malloc(sizeof(T_list_s_e));
	list_worker_init(s->workers);
	list_s_e_init(s->alias);
	list_s_e_init(s->indexes);
	strcpy(s->name,name);
	strcpy(s->dir,dir);
	s->status = W_ONLINE;
	s->id = id;
	s->version = version;
	s->size = size;
}

T_list_worker *site_get_workers(T_site *s){
	return s->workers;
}

unsigned int site_get_id(T_site *s){
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

void site_set_size(T_site *s, unsigned int size){
	s->size = size;
}

unsigned int site_get_size(T_site *s){
	return s->size;
}

unsigned int site_get_real_size(T_site *s){
	return list_worker_size(s->workers);
}

T_list_s_e *site_get_alias(T_site *s){
	return s->alias;
}

T_list_s_e *site_get_indexes(T_site *s){
	return s->indexes;
}

T_site_status site_get_status(T_site *s){
	return s->status;
}

/*****************************
	 Workers
******************************/
void worker_init(T_worker *w, int id, char *name, char *ip, T_worker_status s){
	strcpy(w->name,name);
	strcpy(w->ip,ip);
	w->id = id;
	w->sites = (T_list_site*)malloc(sizeof(T_list_site));
	list_site_init(w->sites);
	w->laverage = 0.0;
	w->server.sin_addr.s_addr = inet_addr(ip);
	w->server.sin_family = AF_INET;
	w->server.sin_port = htons(3550);
	w->status = s;
	w->last_status = s;
	w->time_change_status = time(0);
	w->is_changed = 0;
	worker_connect(w);
}
int worker_connect(T_worker *w){
	/* Funcion de uso interno */
	/* Intenta conectarse al worker */

	close(w->socket);
	w->socket = socket(AF_INET , SOCK_STREAM , 0);
	if (connect(w->socket , (struct sockaddr *) &(w->server) , sizeof(w->server)) < 0){
		printf("Worker %s NO CONECTA\n",w->name);
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

T_list_site *worker_get_sites(T_worker *w){
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
		list_site_first(w->sites);
		printf("PURGE: Eliminamos sitios logicos\n");
		printf("PURGE: Entrando while\n");
		printf("El worker %s posee %i sitios\n",worker_get_name(w),list_site_size(w->sites));
		while(!list_site_eol(w->sites)){
			printf("Obteniendo sitio\n");
			site = list_site_get(w->sites);
			printf("PURGE: Removiendo worker del sitio\n");
			list_worker_remove_id(site_get_workers(site),worker_get_id(w));
			printf("PURGE: proximo sitio\n");
			list_site_next(w->sites);
		}
		printf("PURGE: borrando lista de sitios del worker\n");
		list_site_erase(w->sites);
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

int worker_add_site(T_worker *w, T_site *s,char *default_domain){
	/* Agrega fisica y logicamente un sitio a un worker.
	 * Si no pudo hacerlo retorna 0 caso contrario 1 */

	char aux[100];
	char *send_message = NULL;
	uint32_t send_message_size;
	char *rcv_message = NULL;
	uint32_t rcv_message_size;
	int ok=1;

	T_s_e *aux_s_e;

	printf("WORKER_ADD_SITE: Entro\n");
	send_message_size = 100;
	send_message = (char *)malloc(send_message_size);
	sprintf(send_message,"A%lu|%s|%s|%i|",site_get_id(s),site_get_name(s),site_get_dir(s),
	site_get_version(s));

	// Armar los alias
	printf("WORKER_ADD_SITE: armamos alias\n");
	list_s_e_first(site_get_alias(s));
	while(!list_s_e_eol(site_get_alias(s))){
		aux_s_e = list_s_e_get(site_get_alias(s));
		printf("alias agregado: %s\n",s_e_get_name(aux_s_e));
		sprintf(aux,"%s,",s_e_get_name(aux_s_e));
		if(send_message_size < strlen(send_message) + strlen(aux) + 2){
			printf("allocando espacio\n");
			send_message_size += 100;
			send_message = (char *)realloc(send_message,send_message_size);
		}
		strcat(send_message,aux);
		list_s_e_next(site_get_alias(s));
	}
	send_message[strlen(send_message)-1] = '|';

	// Armar los indices
	printf("WORKER_ADD_SITE: armamos indices\n");
	list_s_e_first(site_get_indexes(s));
	while(!list_s_e_eol(site_get_indexes(s))){
		aux_s_e = list_s_e_get(site_get_indexes(s));
		printf("index agregado: %s\n",s_e_get_name(aux_s_e));
		sprintf(aux,"%s,",s_e_get_name(aux_s_e));
		if(send_message_size < strlen(send_message) + strlen(aux) + 2){
			send_message_size += 100;
			send_message = (char *)realloc(send_message,send_message_size);
		}
		strcat(send_message,aux);
		list_s_e_next(site_get_indexes(s));
	}

	/* Normalizamos el send_message */
	send_message_size = strlen(send_message)+1;
	send_message = (char *)realloc(send_message,send_message_size);
	
	printf("WORKER_ADd_SITE: enviamos mensaje\n");
	if(worker_send_receive(w,send_message,send_message_size,&rcv_message,&rcv_message_size)){
		if(rcv_message[0] == '1'){
			list_site_add(w->sites,s);
			list_worker_add(site_get_workers(s),w);
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
		list_site_remove_id(worker_get_sites(w),site_get_id(s));
		list_worker_remove_id(site_get_workers(s),worker_get_id(w));
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
		if(send(w->socket,buffer,ROLE_BUFFER_SIZE,0)<0){
			worker_change_status(w,W_UNKNOWN);
			worker_connect(w);
			return 0;
		}
	}

	/* Recibir */
	c=0;
	/* Al menos una recepcion esperamos recibir */
	int_to_4bytes(&c,buffer);
	int_to_4bytes(&c,&(buffer[4]));
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
			*rcv_message=(char *)realloc(*rcv_message,*rcv_message_size);
		}

		_4bytes_to_int(&(buffer[4]),&parce_size);
		memcpy(*rcv_message+c,&(buffer[ROLE_HEADER_SIZE]),parce_size);
		c += parce_size;
	} while (c < *rcv_message_size);
	printf("WORKER RCV: send_message_size=:%u, send_message=%s\n",rcv_message_size,*rcv_message);
	return 1;
}

int worker_sync(T_worker *w, T_list_site *s){
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
	while(pos<rcv_message_size){
		parce_data(rcv_message,'|',&pos,aux);
		site = list_site_find_id(s,atoi(aux));
		if(site){
			list_site_add(w->sites,site);
			list_worker_add(site_get_workers(site),w);
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
	if (connect(p->socket , (struct sockaddr *) &(p->server) , sizeof(p->server)) < 0){
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

void proxy_set_online(T_proxy *p){
	proxy_change_status(p,P_PREPARED);
}

void proxy_set_offline(T_proxy *p){
	proxy_change_status(p,P_OFFLINE);
}

char *proxy_get_name(T_proxy *p){
	return p->name;
}
char *proxy_get_ip(T_proxy *p){
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

void proxy_reconfig(T_proxy *p, T_list_site *sites){
	/* Reconfigura un proxy en base a la informacion
	   de los sitios. Borra toda configuracion anterior.
	   De momento un proxy ve todos los sitios y no unos en particular */

	char send_message[200];
	char *rcv_message=NULL;
	uint32_t rcv_message_size=0;

	sprintf(send_message,"D");
	if(proxy_send_receive(p,send_message,2,&rcv_message,&rcv_message_size)){
		list_site_first(sites);
		while(!list_site_eol(sites)){
			printf("Agregamos el sitio al proxy\n");
			proxy_add_site(p,list_site_get(sites));
			list_site_next(sites);
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
	if(list_worker_size(site_get_workers(s)) == 0){
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
		list_worker_first(site_get_workers(s));
		while(!list_worker_eol(site_get_workers(s))){
			worker = list_worker_get(site_get_workers(s));
			sprintf(aux,"%s,",worker_get_name(worker));
			if(send_message_size < strlen(send_message) + strlen(aux) + 2){
				printf("allocando espacio\n");
				send_message_size += 100;
				send_message = (char *)realloc(send_message,send_message_size);
			}
			strcat(send_message,aux);
			list_worker_next(site_get_workers(s));
		}
		send_message[strlen(send_message)-1] = '|';
		
		printf("Armamos los alias\n");
		list_s_e_first(site_get_alias(s));
		while(!list_s_e_eol(site_get_alias(s))){
			aux_s_e = list_s_e_get(site_get_alias(s));
			printf("alias agregado: %s\n",s_e_get_name(aux_s_e));
			sprintf(aux,"%s,",s_e_get_name(aux_s_e));
			if(send_message_size < strlen(send_message) + strlen(aux) + 2){
				printf("allocando espacio\n");
				send_message_size += 100;
				send_message = (char *)realloc(send_message,send_message_size);
			}
			strcat(send_message,aux);
			list_s_e_next(site_get_alias(s));
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
	 Lista de Workers
******************************/
void list_worker_init(T_list_worker *l){
	l->first = NULL;
	l->actual = NULL;
	l->last = NULL;
	l->size = 0;
}

void list_worker_add(T_list_worker *l, T_worker *w){
	/* Agrega un worker al last de la lista.
	 * El puntero actual no se modifica. Excepto que se
	 * trate del primer elemento en ingresar a la lista
	 * No contempla datos repetidos. Es responsabilidad de
	 * la funcion llamadora */

	list_w_node *new;
	list_w_node *aux;

	new = (list_w_node*)malloc(sizeof(list_w_node));
	new->next = NULL;
	new->data = w;
	l->size++;

	if(l->first == NULL){
		l->first = new;
		l->last = new;
	} else {
		l->last->next = new;
		l->last = new;
	}
}

void list_worker_copy(T_list_worker *l, T_list_worker *l2){
	/* Copia la lista l a l2. Los elementos son el mismo
	 * y no se copian sino que son punteros */
	/* Se supone que l2 esta vacia e inicializada */

	list_w_node *aux;

	aux = l->first;
	while(aux!=NULL){
		list_worker_add(l2,aux->data);
		aux = aux->next;
	}
}

void list_worker_first(T_list_worker *l){
	/* Situa el puntero actual al inicio de la lista */
	l->actual = l->first;
}

void list_worker_next(T_list_worker *l){
	/* Avanza el puntero de actual al siguiente siempre que pueda */
	if(l->actual != NULL){
		l->actual = l->actual->next;
	}
}

T_worker *list_worker_get(T_list_worker *l){
	/* Retorna el elemento donde el puntero actual se
	 * encuentre. */
	if(l->actual!=NULL)
		return l->actual->data;
	else 
		return NULL;
}

T_worker *list_worker_get_first(T_list_worker *l){
	/* Retorna el primer elemento de la lista */
	if(l->first!=NULL)
		return l->first->data;
	else
		return NULL;
}

T_worker *list_worker_get_last(T_list_worker *l){
	/* Retorna el ultimo elemento de la lista */
	if(l->last!=NULL)
		return l->last->data;
	else
		return NULL;
}

unsigned int list_worker_size(T_list_worker *l){
	/* Retorna el largo de la lista */
	return l->size;
}

int list_worker_eol(T_list_worker *l){
	/* Retorna si el puntero actual esta al last
	 * de la lista */
	return (l->actual == NULL);
}

T_worker *list_worker_remove_id(T_list_worker *l, int w_id){
	/* Elimina de la lista el nodo que contiene al worker.
 	 * el puntero al worker es retornado */

	int exist = 0;
	list_w_node *aux;

	printf("REMOVE ---- Removemos worker con id: %i\n",w_id);
	l->actual = l->first;
	while(!exist && l->actual != NULL){
		printf("REMOVE ---- Comparamos %i con %i\n",worker_get_id(l->actual->data),w_id);
		if(worker_get_id(l->actual->data) == w_id){
			printf("list_worker_remove_id: Encontramos el worker a eliminar\n");
			return list_worker_remove(l);
		}
		l->actual = l->actual->next;
	}
	return NULL;
}

T_worker *list_worker_remove(T_list_worker *l){
	/* Elimina de la lista el nodo que contiene el elemento
 	 * apuntado por el punero actual. El objeto dentro del nodo
 	 * no se elimina.
 	 * El puntero actual pasa al nodo siguiente. */

	list_w_node *prio;
	list_w_node *aux;
	T_worker *element;

	if(l->actual != NULL){
		aux = l->first;
		prio = NULL;
		while(aux != l->actual){
			prio = aux;
			aux = aux->next;
		}
		if(prio == NULL){
			l->first = aux->next;
		} else {
			prio->next = aux->next;
		}
		if(aux == l->last){
			l->last = prio;
		}
		l->actual = aux->next;
		element = aux->data;
		free(aux);
		l->size--;
	}
	return element;
}

void list_worker_erase(T_list_worker *l){
	/* Vacia la lista sin eliminar los elementos. */
	list_worker_first(l);
	while(!list_worker_eol(l)){
		list_worker_remove(l);
	}
}

T_worker *list_worker_find_id(T_list_worker *l, int worker_id){
	/* Retorna el worker buscando por su id. Si no
	 * existe retorna null */

	list_w_node *aux;
	int exist = 0;

	aux = l->first;
	while(aux != NULL && !exist){
		exist = (worker_get_id(aux->data) == worker_id);
		if(!exist){ aux = aux->next;}
	}
	if(exist){
		return aux->data;
	} else {
		return NULL;
	}
}

void list_worker_sort_by_load(T_list_worker *l,int des){
	/* Ordena la lista por la carga (load average) del worker */

	T_worker *worker1, *worker2;
	int i,j;

	for(i=0;i<l->size - 1;i++){
		list_worker_first(l);
		for(j=1;j<l->size - i;j++){
			worker1 = l->actual->data;
			worker2 = l->actual->next->data;
			if(des){
				if(worker_get_load(worker1) < worker_get_load(worker2)){
					l->actual->data = worker2;
					l->actual->next->data = worker1;
				}
			} else {
				if(worker_get_load(worker1) > worker_get_load(worker2)){
					l->actual->data = worker2;
					l->actual->next->data = worker1;
				}
			}
			list_worker_next(l);
		}
	}
}



void list_worker_sort_by_site(T_list_worker *l,int des){
	/* Ordena la lista por cantidad de sitios */

	T_worker *worker1, *worker2;
	int i,j;
	for(i=1;i<l->size;i++){
		l->actual = l->first;
		for(j=1;j<=(l->size - i);j++){
			worker1 = l->actual->data;
			worker2 = l->actual->next->data;
			if(des){
				if((list_site_size(worker_get_sites(worker1))) <
				   (list_site_size(worker_get_sites(worker2)))){
					l->actual->data = worker2;
					l->actual->next->data = worker1;
				}
			} else {
				if((list_site_size(worker_get_sites(worker1))) >
				   (list_site_size(worker_get_sites(worker2)))){
					l->actual->data = worker2;
					l->actual->next->data = worker1;
				}
			}
			list_worker_next(l);
		}
	}
}

void list_worker_print(T_list_worker *l){
	l->actual = l->first;
	printf("PRINT WORKER\n");
	while(l->actual != NULL){
		printf("Print Worker: %s\n",worker_get_name(l->actual->data));
		l->actual = l->actual->next;
	}
	printf("--------------\n");
}

/*****************************
	 Lista de Sitios
******************************/

void list_site_init(T_list_site *l){
	l->first = NULL;
	l->actual = NULL;
	l->last = NULL;
	l->size = 0;
}

void list_site_add(T_list_site *l, T_site *s){
	/* Agrega un sitio al final de la lista.
	 * El puntero actual no se modifica. Excepto que se
	 * trate del primer elemento en ingresar a la lista
	 * No contempla datos repetidos. Es responsabilidad de
	 * la funcion llamadora */

	list_s_node *new;
	list_s_node *aux;

	new = (list_s_node*)malloc(sizeof(list_s_node));
	new->next = NULL;
	new->data = s;
	l->size++;

	if(l->first == NULL){
		l->first = new;
		l->last = new;
	} else {
		l->last->next = new;
		l->last = new;
	}
}

void list_site_first(T_list_site *l){
	/* Situa el puntero actual al inicio de la lista */
	l->actual = l->first;
}

void list_site_next(T_list_site *l){
	/* Avanza el puntero de actual al siguiente siempre que pueda */
	if(l->actual != NULL){
		l->actual = l->actual->next;
	}
}

void list_site_copy(T_list_site *l, T_list_site *l2){
	/* Copia la lista l a l2. Los elementos son el mismo
	 * y no se copian sino que son punteros */
	/* Se supone que l2 esta vacia e inicializada */

	list_s_node *aux;

	aux = l->first;
	while(aux!=NULL){
		list_site_add(l2,aux->data);
		aux = aux->next;
	}
}


T_site *list_site_get(T_list_site *l){
	/* Retorna el elemento donde el puntero actual se
	 * encuentre. */
	return l->actual->data;
}

T_site *list_site_find_id(T_list_site *l, unsigned int site_id){
	/* busca y retorna el sitio que posea el id. Si no existe
	 * returna null. No mueve el indice actual de la lista */

	list_s_node *aux;
	int exist = 0;

	aux = l->first;
	while(aux!=NULL && !exist){
		exist = (site_get_id(aux->data) == site_id);
		if(!exist){ aux = aux->next;}
	}
	if(exist){
		return aux->data;
	} else {
		return NULL;
	}
}

unsigned int list_site_size(T_list_site *l){
	/* Retorna el largo de la lista */
	return l->size;
}

int list_site_eol(T_list_site *l){
	/* Retorna si el puntero actual esta al last
	 * de la lista */
	return (l->actual == NULL);
}

T_site *list_site_remove(T_list_site *l){
	/* remueve de la lista el elemento apuntado por el punero actual.
 	 * El puntero actual pasa al nodo siguiente */
	/* EL ELEMENTO SE RETORNA. SOLO SE LO REMUEVE DE LA LISTA */

	list_s_node *prio;
	list_s_node *aux;
	T_site *element = NULL;

	if(l->actual != NULL){
		aux = l->first;
		prio = NULL;
		while(aux != l->actual){
			prio = aux;
			aux = aux->next;
		}
		if(prio == NULL){
			l->first = aux->next;
		} else {
			prio->next = aux->next;
		}
		if(aux == l->last){
			l->last = prio;
		}
		l->actual = aux->next;
		element = aux->data;
		free(aux);
		l->size--;
	}
	return element;
}

T_site *list_site_remove_id(T_list_site *l, unsigned int id){
	/* remueve de la lista el elemento apuntado por el punero actual.
 	 * El puntero actual pasa al nodo siguiente */
	/* EL ELEMENTO SE RETORNA. SOLO SE LO REMUEVE DE LA LISTA */

	int exist = 0;
	list_s_node *aux;

	l->actual = l->first;
	while(!exist && l->actual != NULL){
		if(site_get_id(l->actual->data) == id){
			printf("list_site_remove_id: Encontramos el site a eliminar\n");
			return list_site_remove(l);
		}
		l->actual = l->actual->next;
	}
	return NULL;
}


void list_site_erase(T_list_site *l){
	/* Vacia la lista sin eliminar los elementos. */
	list_site_first(l);
	while(!list_site_eol(l)){
		list_site_remove(l);
	}
}

void list_site_print(T_list_site *l){
	l->actual = l->first;
	printf("PRINT SITES\n");
	while(l->actual != NULL){
		printf("Print Site: %s\n",site_get_name(l->actual->data));
		l->actual = l->actual->next;
	}
	printf("--------------\n");
}

/*****************************
	 Lista de Proxys
******************************/

void list_proxy_init(T_list_proxy *l){
	l->first = NULL;
	l->actual = NULL;
	l->last = NULL;
	l->size = 0;
}

void list_proxy_add(T_list_proxy *l, T_proxy *w){
	/* Agrega un proxy al last de la lista.
	 * El puntero actual no se modifica. Excepto que se
	 * trate del primer elemento en ingresar a la lista */

	list_p_node *new;
	list_p_node *aux;

	new = (list_p_node*)malloc(sizeof(list_p_node));
	new->next = NULL;
	new->data = w;
	l->size++;

	if(l->first == NULL){
		l->first = new;
		l->last = new;
	} else {
		l->last->next = new;
		l->last = new;
	}
}

void list_proxy_first(T_list_proxy *l){
	/* Situa el puntero actual al inicio de la lista */
	l->actual = l->first;
}

void list_proxy_next(T_list_proxy *l){
	/* Avanza el puntero de actual al siguiente siempre que pueda */
	if(l->actual != NULL){
		l->actual = l->actual->next;
	}
}

T_proxy *list_proxy_get(T_list_proxy *l){
	/* Retorna el elemento donde el puntero actual se
	 * encuentre. */
	return l->actual->data;
}

unsigned int list_proxy_size(T_list_proxy *l){
	/* Retorna el largo de la lista */
	return l->size;
}

int list_proxy_eol(T_list_proxy *l){
	/* Retorna si el puntero actual esta al last
	 * de la lista */
	return (l->actual == NULL);
}

void list_proxy_remove(T_list_proxy *l){
	/* Elimina del alista el elemento apuntado por el punero actual.
 	 * El puntero actual pasa al nodo siguiente */

	list_p_node *prio;
	list_p_node *aux;

	if(l->actual != NULL){
		aux = l->first;
		prio = NULL;
		while(aux != l->actual){
			prio = aux;
			aux = aux->next;
		}
		if(prio == NULL){
			l->first = aux->next;
		} else {
			prio->next = aux->next;
		}
		if(aux == l->last){
			l->last = prio;
		}
		l->actual = aux->next;
		free(aux);
		l->size--;
	}
}

void list_proxy_erase(T_list_proxy *l){
	/* Vacia la lista sin eliminar los elementos. */
	list_proxy_first(l);
	while(!list_proxy_eol(l)){
		list_proxy_remove(l);
	}
}
