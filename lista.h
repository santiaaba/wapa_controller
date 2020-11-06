#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef LISTA_H
#define LISTA_H

typedef struct lista T_lista;

typedef struct lista_node {
	void *data;
	struct lista_node *next;
} lista_node;

struct lista{
	int e_size;
	unsigned int size;
	lista_node *first;
	lista_node *last;
	lista_node *actual;
	pthread_mutex_t lock;
};

void lista_init(T_lista *l, int e_size);
void lista_add(T_lista *l, void *e);
void *lista_get(T_lista *l);
void *lista_get_first(T_lista *l);
void *lista_get_last(T_lista *l);
void lista_first(T_lista *l);
void lista_next(T_lista *l);
unsigned int lista_size(T_lista *l);
unsigned int lista_eol(T_lista *l);
void lista_copy(T_lista *l, T_lista *t);
void *lista_remove(T_lista *l);
void lista_clean(T_lista *l,void(*clean)(void**));
void *lista_exclude(T_lista *l,int (*find_id)(void*), int value);
void lista_erase(T_lista *l);
void *lista_find(T_lista *l, int (*find_id)(void*), int value);
void *lista_sort(T_lista *l, float (*get_value)(void*), int asc);

#endif
