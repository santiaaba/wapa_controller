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
void site_init(T_site *s, char *name, unsigned int id, unsigned int userid,
	       unsigned int susc, unsigned int version, unsigned int size){
	s->workers = (T_list_worker*)malloc(sizeof(T_list_worker));
	list_worker_init(s->workers);
	strcpy(s->name,name);
	s->status = W_ONLINE;
	s->id = id;
	s->userid = userid;
	s->susc = susc;
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

char *site_get_name(T_site *s){
	return s->name;
}

unsigned int site_get_userid(T_site *s){
	return s->userid;
}

unsigned int site_get_susc(T_site *s){
	return s->susc;
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

char *site_get_alias(T_site *s){
	return s->alias;
}

T_site_status site_get_status(T_site *s){
	return s->status;
}

/*****************************
         Workers
******************************/
void worker_init(T_worker *w, int id, char *name, char *ip, T_worker_status s){
	w->id = id;
	w->sites = (T_list_site*)malloc(sizeof(T_list_site));
	list_site_init(w->sites);
	strcpy(w->name,name);
	strcpy(w->ip,ip);
	w->socket = socket(AF_INET , SOCK_STREAM , 0);
	printf("La ip del worker %s es %s\n",name,ip);
	w->server.sin_addr.s_addr = inet_addr(ip);
	w->server.sin_family = AF_INET;
	w->server.sin_port = htons(3550);
	w->status = s;
	w->last_status = s;
	w->time_change_status = time(0);
	worker_connect(w);
}
int worker_connect(T_worker *w){
	/* Funcion de uso interno */
	/* Intenta conectarse al worker */

	printf("worker %s: Conectamos de nuevo!!!\n",worker_get_name(w));
	if (connect(w->socket , (struct sockaddr *) &(w->server) , sizeof(w->server)) < 0){
		printf("worker-connect: Problemas para conectar\n");
		w->status = W_UNKNOWN;
	}
}

int worker_end_connect(T_worker *w){
	/* Funcion de uso interno */
	/* Finaliza la coneccion contra el worker fisico */
	close(w->socket);
}

void worker_change_status(T_worker *w, T_worker_status s){
	/* funcion de uso interno */
	w->time_change_status = time(0);
	w->last_status = w->status;
	w->status = s;
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

void worker_set_online(T_worker *w){
	worker_change_status(w,W_PREPARED);
}

void worker_set_offline(T_worker *w){
	worker_change_status(w,W_OFFLINE);
	worker_end_connect(w);
}

char *worker_get_name(T_worker *w){
	return w->name;
}

char *worker_get_ip(T_worker *w){
	return w->ip;
}

void worker_purge(T_worker *w){
	/* Se desasignan todos los sitios del worker */

	T_site *site;

	list_site_first(w->sites);
	while(list_site_eol(w->sites)){
		site = list_site_get(w->sites);
		list_worker_remove(site_get_workers(site));
		list_site_next(w->sites);
	}
	list_site_erase(w->sites);
}

void worker_check(T_worker *w){
	/* Verifica un worker. Actualiza el estado del mismo */

	char buffer_rx[BUFFERSIZE];

	/* Verificamos si responde correctamente */
	/* Si esta OFFLINE no hacemos nada. Sigue en OFFLINE */
	if(w->status != W_OFFLINE){
		if(!worker_send_recive(w,"C",buffer_rx)){
			/* No responde */
			worker_change_status(w,W_UNKNOWN);
		} else {
			if(buffer_rx[4] == 1){
				/* Responde y pasa el chequeo */
				if(w->status == W_BROKEN || w->status == W_UNKNOWN){
					/* Si estaba en BROKEN o UNKNOWN pasa a PREPARED */
					worker_change_status(w,W_PREPARED);
				} else {
					if((w->status == W_PREPARED) &&
					   (difftime(w->time_change_status,time(0)) > TIMEONLINE)){
					         /* Responde, pasa el chequeo, 
						  * ya estaba en PREPARED y paso el tiempo */
						 worker_change_status(w,W_ONLINE);
					}
				}
			} else {
				 worker_change_status(w,W_BROKEN);
			}
		}
	}
}

int worker_add_site(T_worker *w, T_site *s){
	/* Agrega fisica y logicamente un sitio a un worker.
	 * Si no pudo hacerlo retorna 0 caso contrario 1 */

	char command[BUFFERSIZE];
	char aux[50];
	char aux2[4];	//Para codificar los unsigned int;
	char buffer_rx[BUFFERSIZE];

	/* OJO.. en la siguiente linea puede que superemos el buffer
	 * a causa de los alias. Habria que enviarlos de otra forma */
	sprintf(command,"1|%s|%s|%s|%s\0",site_get_name(s),
	site_get_userid(s),site_get_susc(s),site_get_alias(s));
	if(worker_send_recive(w,command,buffer_rx)){
		list_site_add(w->sites,s);
		list_worker_add(site_get_workers(s),w);
		return 1;
	} else {
		// Problemas para contactar al worker
		return 0;
	}
}

int worker_send_recive(T_worker *w, char *command, char *buffer_rx){
	/* Se conecta al worker, envia un comando y espera la respuesta */

	int cant_bytes;

	cant_bytes = send(w->socket,command, BUFFERSIZE,0);
	printf("send_recive - enviando(%i): %s\n",cant_bytes,command);
	if(cant_bytes <0){
		/* Fallo la conectividad contra el worker */
		worker_change_status(w,W_UNKNOWN);
		/* Reintentamos conectar */
		worker_connect(w);
		return 0;
	}
	cant_bytes = recv(w->socket,buffer_rx,BUFFERSIZE,0);
	if(cant_bytes<0){
		/* Fallo la conectividad contr el worker */
		worker_change_status(w,W_UNKNOWN);
		/* Reintentamos conectar */
		worker_connect(w);
		return 0;
	}
	printf("send_recive - recibido(%i): %s\n",cant_bytes,buffer_rx);
	return 1;
}

int worker_reconnect(T_worker *w){
	/* funcion de uso interno */
	printf("Reconectamos %s\n",worker_get_name(w));
	worker_change_status(w,W_UNKNOWN);
	worker_connect(w);
	return 0;
}

int worker_sync(T_worker *w, T_list_site *s){
	/* Se conecta al worker, obtiene el listado
	 * de sitios y actualiza las estructuras */

	char buffer_rx[BUFFERSIZE];
	char buffer_tx[BUFFERSIZE];
	int nextdata = 0;	//Fin de transmision
	int pos = 2;		// Ya que en 2 se indica si hay mas datos luego
	char aux[10];
	T_site *site;

	/*En este procedimiento no podemos utilizar el metodo worker_send_recive */

	printf("Entramos a Sync\n");
	if(send(w->socket,"G\0", BUFFERSIZE,0)<0){
		printf("Intentamos reconectar\n");
		return worker_reconnect(w);
	}
	do{
		//recibimos la primer holeada de datos
		if(recv(w->socket,buffer_rx,BUFFERSIZE,0)<0){
			printf("problemas de conectividad\n");
			return worker_reconnect(w);
		} else {
			printf("La primer oleada de datos es: -%s-\n",buffer_rx);
			// El primer valor de los datos es un 1 o un 0. Es un 1 si hay mas datos
			// para recibir luego de estos
			parce_data(buffer_rx,&pos,aux);
			nextdata = atoi(aux);
			printf("pos deberia estar en 4: %i\n",pos);
			while(strlen(buffer_rx) > 0 && pos < strlen(buffer_rx)){
				printf("POS: %i\n",pos);
				parce_data(buffer_rx,&pos,aux);
				printf("sitio obtenido: %s\n",aux);
				site = list_site_find_id(s,atoi(aux));
				list_site_add(w->sites,site);
				list_worker_add(site_get_workers(site),w);
			}
			if(nextdata == 1){
				if(send(w->socket,"1\0",BUFFERSIZE,0)<0){
		                        printf("problemas de conectividad\n");
        		                return worker_reconnect(w);
				}
			}
		}
	} while(nextdata);
	printf("-- Terminamos SYNC\n");
}

/*****************************
         Proxys
******************************/
void proxy_init(T_proxy *p, char *name, char *ip){
	strcpy(p->name,name);
	strcpy(p->ip,ip);
}
char *proxy_get_name(T_proxy *p){
	return p->name;
}
char *proxy_get_ip(T_proxy *p){
	return p->ip;
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
	return l->actual->data;
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

T_worker *list_worker_remove(T_list_worker *l){
	/* Elimina del alista el elemento apuntado por el punero actual.
 	 * El puntero actual pasa al nodo siguiente */

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

	int exist = 0;

	list_worker_first(l);
	while(!list_worker_eol(l) && !exist){
		exist = (worker_get_id(l->actual->data) == worker_id);
		list_worker_next(l);
	}
	/* Si se encontr se retorna. Sino l->actual->data deberia ser NULL */
	return l->actual->data;
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

void list_site_add(T_list_site *l, T_site *w){
	/* Agrega un worker al last de la lista.
	 * El puntero actual no se modifica. Excepto que se
	 * trate del primer elemento en ingresar a la lista
	 * No contempla datos repetidos. Es responsabilidad de
	 * la funcion llamadora */

	list_s_node *new;
	list_s_node *aux;

	new = (list_s_node*)malloc(sizeof(list_s_node));
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
	}
	return element;
}

void list_site_erase(T_list_site *l){
	/* Vacia la lista sin eliminar los elementos. */
	list_site_first(l);
	while(!list_site_eol(l)){
		list_site_remove(l);
	}
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
	}
}

void list_proxy_erase(T_list_proxy *l){
	/* Vacia la lista sin eliminar los elementos. */
	list_proxy_first(l);
	while(!list_proxy_eol(l)){
		list_proxy_remove(l);
	}
}


