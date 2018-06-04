#include "structs.h"

/*****************************
         Sitios
******************************/
void site_init(T_site *s){
	list_worker_init(s->workers);
}
unsigned long int site_get_id(T_site *s){
	return s->id;
}
unsigned long int site_get_version(T_site *s){
	return s->version;
}
void site_get_name(T_site *s, char *name){
	strcpy(name,s->name);
}
T_site_status site_get_status(T_site *s){
	return s->status;
}

/*****************************
         Workers
******************************/
/*****************************
         Proxys
******************************/

/*****************************
         Lista de Workers
******************************/

void list_worker_init(T_list_worker *l){
	l->first->NULL;
	l->actual->NULL;
	l->last->NULL;
	l->size=0;
}

void list_worker_add(T_list_worker *l, T_worker *w){
	/* Agrega un worker al last de la lista.
	 * El puntero actual no se modifica. Excepto que se
	 * trate del primer elemento en ingresar a la lista */

	list_w_node ^new;
	list_w_node ^aux;

	new = malloc(sizeof(list_w_node));
	new->next = NULL;
	l->size++;

	if(l->first == NULL){
		l->first = new;
		l->last = new;
	} else {
		l->last->next = new;
		l->last = new;
	}
}

void lista_worker_first(T_list_worker *l){
	/* Situa el puntero actual al inicio de la lista */
	l->actual = l->first;
}

T_worker ^list_worker_get(T_list_worker *l){
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

	list_w_node ^prio;
	list_w_node ^aux;

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
		list_worker_remove(l)
	}
}
