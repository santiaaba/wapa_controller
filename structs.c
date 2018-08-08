#include "structs.h"
#include "string.h"

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
	  Alias 
******************************/
void alias_init(T_alias *a, unsigned int id, char *name){
	a->id = id;
	a->name = malloc(strlen(name));
	strcpy(a->name,name);
	printf("Nombre copiado %s = %s\n",name,a->name);
}

char *alias_get_name(T_alias *a){
	return a->name;
}

unsigned int alias_get_id(T_alias *a){
	return a->id;
}

/*****************************
	 Sitios
******************************/
void site_init(T_site *s, char *name, unsigned int id, unsigned int userid,
	       unsigned int susc, unsigned int version, unsigned int size){
	s->workers = (T_list_worker*)malloc(sizeof(T_list_worker));
	s->alias = (T_list_alias*)malloc(sizeof(T_list_alias));
	list_worker_init(s->workers);
	list_alias_init(s->alias);
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

T_list_alias *site_get_alias(T_site *s){
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

	close(w->socket);
	w->socket = socket(AF_INET , SOCK_STREAM , 0);
	if (connect(w->socket , (struct sockaddr *) &(w->server) , sizeof(w->server)) < 0){
		w->status = W_UNKNOWN;
	}
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
	char aux[100];
	char aux2[100];
	
	itowstatus(w->last_status,aux);
	itowstatus(s,aux2);
	if(w->status != s){
		w->time_change_status = (unsigned long)time(0);
	}
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

T_list_site *worker_get_sites(T_worker *w){
	return w->sites;
}

void worker_purge(T_worker *w){
	/* Se desasignan todos los sitios del worker.
	 * Se eliminan tambien los archivos fisicos */

	T_site *site;
	char buffer_rx[BUFFERSIZE];

	printf("PURGE: Eliminamos sitios logicos\n");
	list_site_first(w->sites);
	printf("PURGE: Entrando while\n");
	while(!list_site_eol(w->sites)){
		printf("Obteniendo sitio\n");
		site = list_site_get(w->sites);
		printf("PURGE: Removiendo worker del sitio\n");
		list_worker_remove(site_get_workers(site));
		printf("PURGE: proximo sitio\n");
		list_site_next(w->sites);
	}
	printf("PURGE: borrando lista de sitios del worker\n");
	list_site_erase(w->sites);

	/* Una vez eliminados todos los sitios, procedemos
 	 * a pedirle al worker fisico que elimine los archivos */
	printf("PURGE: Eliminamos sitios fisicos\n");
	worker_send_recive(w,"P",buffer_rx);
}

void worker_check(T_worker *w){
	/* Verifica un worker. Actualiza el estado del mismo */

	char buffer_rx[BUFFERSIZE];

	/* Verificamos si responde correctamente */
	/* Si esta OFFLINE no hacemos nada. Sigue en OFFLINE */
	printf("Tiempo entre estados %lu - %lu = %lu\n",(unsigned long)time(0),(unsigned long)w->time_change_status,(unsigned long)time(0) - (unsigned long)w->time_change_status);
	if(w->status != W_OFFLINE){
		if(!worker_send_recive(w,"C",buffer_rx)){
			/* No responde */
			worker_change_status(w,W_UNKNOWN);
		} else {
			printf("CHECK: -%s-\n",buffer_rx);
			if(buffer_rx[0] == '1'){
				/* Responde y pasa el chequeo */
				if(w->status == W_ONLINE){
					/* Continua ON_LINE */
					worker_change_status(w,W_ONLINE);
				} else {
					if(w->status == W_BROKEN || w->status == W_UNKNOWN){
						/* Si estaba en BROKEN o UNKNOWN pasa a PREPARED */
						worker_change_status(w,W_PREPARED);
					} else {
						if((w->status == W_PREPARED) &&
						   ((unsigned long)time(0) - w->time_change_status) > TIMEONLINE){
							 /* Responde, pasa el chequeo, 
							  * ya estaba en PREPARED y paso el tiempo */
							 worker_change_status(w,W_ONLINE);
						} else {
							/* Continua en PREPARED */
							 worker_change_status(w,W_PREPARED);
						}
					}
				}
			} else {
				printf("worker %s ROTO!\n",worker_get_name(w));
				worker_change_status(w,W_BROKEN);
			}
		}
	}
}

int worker_add_site(T_worker *w, T_site *s,char *default_domain){
	/* Agrega fisica y logicamente un sitio a un worker.
	 * Si no pudo hacerlo retorna 0 caso contrario 1 */

	char aux[512];
	char aux2[4];	//Para codificar los unsigned int;
	char buffer_rx[BUFFERSIZE];
	char buffer_tx[BUFFERSIZE];
	T_alias *alias;

	/* OJO.. en la siguiente linea puede que superemos el buffer
	 * a causa de los alias. Habria que enviarlos de otra forma */
	printf("OBTENEMOS %lu\n",site_get_susc(s));

	/* hay algun problema con el dato en la posicion de la suscripcion con
	 * comando siguiente. Asi que debemos buscar otra forma 
 	 * sprintf(buffer_tx,"A|%s|%lu|%lu|%lu|%lu|%s",site_get_name(s),site_get_id(s),
	site_get_version(s),site_get_susc(s),site_get_userid(s),default_domain);
	*/
	sprintf(aux,"A|%s|%lu|%lu|%lu|",site_get_name(s),site_get_id(s),
	site_get_version(s),site_get_susc(s));
	strcpy(buffer_tx,aux);
	sprintf(aux,"%lu|%s",site_get_userid(s),default_domain);
	strcat(buffer_tx,aux);

	if(worker_send_recive(w,buffer_tx,buffer_rx)){
		if(buffer_rx[0] == '1'){
			printf("COmenzamos con los alias!!!\n");
			/* Enviamos los alias. Puede que requieran varios envios */
			list_alias_first(site_get_alias(s));
			strcpy(buffer_tx,"0|");		//El 0 indica que no habria mas datos a enviar
			while(!list_alias_eol(site_get_alias(s))){
				alias = list_alias_get(site_get_alias(s));
				printf("Procesando alias: %s\n",alias_get_name(alias));
				if(strlen(alias_get_name(alias)) +
				   strlen(buffer_tx) + 2 > BUFFERSIZE){
					// Buffer lleno. Enviamos lo que tenemos
					// Hay mas para enviar luego asi que cambiamos el primer byte a 1
					buffer_tx[0] = '1';
					if(!worker_send_recive(w,buffer_tx,buffer_rx)){
						//Falla del worker al recibir los datos
						return 0;
					}
					strcpy(buffer_tx,"0|");	//El 0 indica que no habria mas datos a enviar
				}
				//seguimos publando el buffer
				strcat(buffer_tx,alias_get_name(alias));
				strcat(buffer_tx,"|");
				list_alias_next(site_get_alias(s));
			}
			// Enviamos los alias remanentes. Puede que este vacio
			if(!worker_send_recive(w,buffer_tx,buffer_rx)){
				//Falla del worker al recibir los datos
				return 0;
			}
			list_site_add(w->sites,s);
			list_worker_add(site_get_workers(s),w);
			return 1;
		}  else {
			return 0;
		}
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
	if(exist){
		return l->actual->data;
	} else {
		return NULL;
	}
}

void list_worker_sort(T_list_worker *l){
	/* Ordena la lista de menor a mayor por cantidad de
 	 * sitios en cada worker */

	T_worker *worker1, *worker2;
	int i,j;

	for(i=0;i<l->size - 1;i++){
		list_worker_first(l);
		for(j=1;j<l->size - i;j++){
			worker1 = l->actual->data;
			worker2 = l->actual->next->data;
			if((list_site_size(worker_get_sites(worker1))) >
			   (list_site_size(worker_get_sites(worker2)))){
				l->actual->data = worker2;
				l->actual->next->data = worker1;
			}
			list_worker_next(l);
		}
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

/*****************************
*	 Lista de alias
******************************/
void list_alias_init(T_list_alias *l){
	l->first = NULL;
	l->actual = NULL;
	l->last = NULL;
	l->size = 0;
}

void list_alias_add(T_list_alias *l, T_alias *a){
	list_a_node *new;
	list_a_node *aux;

	new = (list_a_node*)malloc(sizeof(list_a_node));
	new->next = NULL;
	new->data = a;
	l->size++;

	if(l->first == NULL){
		l->first = new;
		l->last = new;
	} else {
		l->last->next = new;
		l->last = new;
	}
}

void list_alias_first(T_list_alias *l){
	l->actual = l->first;
}

void list_alias_next(T_list_alias *l){
	if(l->actual != NULL){
		l->actual = l->actual->next;
	}
}

T_alias *list_alias_get(T_list_alias *l){
	return l->actual->data;
}

unsigned int list_alias_size(T_list_alias *l){
	return l->size;
}

int list_alias_eol(T_list_alias *l){
	return (l->actual == NULL);
}

T_alias *list_alias_remove(T_list_alias *l){
	list_a_node *prio;
	list_a_node *aux;
	T_alias *element;

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
