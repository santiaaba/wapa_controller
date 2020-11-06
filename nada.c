#include "vstring.h"
#include <stdlib.h>
#include <string.h>

void list_string_init(T_list_string *l){
	l->first = NULL;
	l->actual = NULL;
	l->last = NULL;
	l->size = 0;
}

void list_string_add(T_list_string *l, char *s){

	char *svt = malloc(strlen(s) + 1);
	list_v_node *new;
	list_v_node *aux;

	new = (list_v_node*)malloc(sizeof(list_v_node));
	new->next = NULL;
	new->data = svt;
	l->size++;

	if(l->first == NULL){
		l->first = new;
		l->last = new;
	} else {
		l->last->next = new;
		l->last = new;
	}
}

char *list_string_get(T_list_string *l){
	return l->actual->data;
	
}
void list_string_first(T_list_string *l){
	l->actual = l->first;
}
void list_string_next(T_list_string *l){
	if(l->actual != NULL){
		l->actual = l->actual->next;
	}
}
unsigned int list_string_size(T_list_string *l){
	return l->size;
}
int list_string_eol(T_list_string *l){
	return (l->actual == NULL);
}
char *list_string_remove(T_list_string *l){

	list_v_node *prio;
	list_v_node *aux;
	char *element = NULL;

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
