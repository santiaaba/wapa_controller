#include "string_element.h"

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

unsigned int s_e_get_id(T_s_e *a){
	return a->id;
}

/*****************************
	Lista de s_e
 ******************************/
void list_s_e_init(T_list_s_e *l){
	l->first = NULL;
	l->actual = NULL;
	l->last = NULL;
	l->size = 0;
}

void list_s_e_add(T_list_s_e *l, T_s_e *a){
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

void list_s_e_first(T_list_s_e *l){
	l->actual = l->first;
}

void list_s_e_next(T_list_s_e *l){
	if(l->actual != NULL){
		l->actual = l->actual->next;
	}
}

T_s_e *list_s_e_get(T_list_s_e *l){
	return l->actual->data;
}

unsigned int list_s_e_size(T_list_s_e *l){
	return l->size;
}

int list_s_e_eol(T_list_s_e *l){
	return (l->actual == NULL);
}

T_s_e *list_s_e_remove(T_list_s_e *l){
	list_a_node *prio;
	list_a_node *aux;
	T_s_e *element;

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
