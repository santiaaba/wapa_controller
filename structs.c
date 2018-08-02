#include "structs.h"

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
void worker_init(T_worker *w, char *name, char *ip, T_worker_status s){
	w->sites = (T_list_site*)malloc(sizeof(T_list_site));
	list_site_init(w->sites);
	strcpy(w->name,name);
	strcpy(w->ip,ip);
	w->socket = socket(AF_INET , SOCK_STREAM , 0);
	w->server.sin_addr.s_addr = inet_addr(ip);
	w->server.sin_family = AF_INET;
	w->server.sin_port = htons(3550);
	w->status = s;
	w->prio_status = s;
	w->time_change_status = time(0);
}
int worker_connect(T_worker *w){
	/* Funcion de uso interno */
	/* Intenta conectarse al worker */

	if (connect(w->socket , (struct sockaddr *) &(w->server) , sizeof(w->server)) < 0){
		printf("Se pudo conectar\n");
	} else {
		printf("Problemas para conectar\n");
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
	w->prio_status = w->status;
	w->status = s;
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

T_worker_status worker_get_status(T_worker *w){
	return w->status;
}

void worker_check(T_worker *w){
	/* Verifica un worker. Actualiza el estado del mismo */

	char buffer_rx[BUFFERSIZE];

	/* Verificamos si responde correctamente */
	/* Si esta OFFLINE no hacemos nada. Sigue en OFFLINE */
	if(w->status != W_OFFLINE){
		if(!worker_send_recive(w,"S",buffer_rx)){
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
	/* El task_id de momento no se implementa
 	 * pero se reservan los primeros 4 bytes para ello */
	char *buffer_tx[BUFFERSIZE];

	strcpy(buffer_tx,"0000|");
	strcat(buffer_tx,command);
	if(send(w->socket,buffer_tx, BUFFERSIZE,0)<0){
		/* Fallo la conectividad contra el worker */
		worker_change_status(w,W_UNKNOWN);
		/* Reintentamos conectar */
		worker_connect(w);
		return 0;
	}
	if(recv(w->socket,buffer_rx,BUFFERSIZE,0)<0){
		/* Fallo la conectividad contr el worker */
		worker_change_status(w,W_UNKNOWN);
		/* Reintentamos conectar */
		worker_connect(w);
		return 0;
	}
	return 1;
}

int worker_sync(T_worker *w, T_list_site *s){
	/* Se conecta al worker, obtiene el listado
	 * de sitios y actualiza las estructuras */

	char buffer_rx[BUFFERSIZE];
	char aux[10];
	int i,j, cant_sites;
	T_site *site;
	unsigned int site_id;
	unsigned int last_site_id;

	/* Si last_site_id es distinto de 0 significa que hay mas datos */
	do{
		if(worker_send_recive(w,"0000G",buffer_rx)){
			/* Obtenemos el last_id */
			i=5;
			for(j=0;j<4;j++){ aux[j] = buffer_rx[i+j];}
			aux[4] = '\0';
			last_site_id = strtoul(aux);
			/* Obtenemos la cantidad de sitios que vienen en la respuesta */

			aux[0] = buffer_rx[9]; aux[1] = buffer_rx[10]; aux[2] = '\0';

			cant_sites = strtoui(aux);
			i = 11;
			while(cant_sites != 0){
				for(j=0;j<4;j++){ aux[j] = buffer_rx[i+j];}
	                        aux[4] = '\0';
				i + 4;
				site_id = atoi(aux);
				/* Asignamos este sitio al worker en cuestion */
				site = list_site_find_site_id(s,site_id);
				list_site_add(w->sites,site);
		                list_worker_add(site_get_workers(site),w);
				cant_sites--;
			}
		} else {
			return 0;
		}
	}while(last_site_id != 0);
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
	 * trate del primer elemento en ingresar a la lista */

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
	if(l->actual->next != NULL){
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

void list_worker_remove(T_list_worker *l){
	/* Elimina del alista el elemento apuntado por el punero actual.
 	 * El puntero actual pasa al nodo siguiente */

	list_w_node *prio;
	list_w_node *aux;

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

void list_worker_erase(T_list_worker *l){
	/* Vacia la lista sin eliminar los elementos. */
	list_worker_first(l);
	while(!list_worker_eol(l)){
		list_worker_remove(l);
	}
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
	 * trate del primer elemento en ingresar a la lista */

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
	if(l->actual->next != NULL){
		l->actual = l->actual->next;
	}
}

T_site *list_site_get(T_list_site *l){
	/* Retorna el elemento donde el puntero actual se
	 * encuentre. */
	return l->actual->data;
}

T_site *list_site_find_site_id(T_list_site *l, unsigned int site_id){
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

void list_site_remove(T_list_site *l){
	/* Elimina del alista el elemento apuntado por el punero actual.
 	 * El puntero actual pasa al nodo siguiente */

	list_s_node *prio;
	list_s_node *aux;

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
	if(l->actual->next != NULL){
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


