#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef STRING_ELEMENT_H
#define STRING_ELEMENT_H

typedef struct list_s_e T_list_s_e;

/*****************************
 *      String Element
 *****************************/
typedef struct {
        char *name;
        unsigned int id;
} T_s_e;

void s_e_init(T_s_e *a, unsigned int id, char *name);
void s_e_free(T_s_e **a);
char *s_e_get_name(T_s_e *a);
unsigned int s_e_get_id(T_s_e *a);
void s_e_set_name(T_s_e *a);

/*****************************
 *         Lista de s_e
 *******************************/
typedef struct a_node {
        T_s_e *data;
        struct a_node *next;
} list_a_node;

struct list_s_e {
        unsigned int size;
        list_a_node *first;
        list_a_node *last;
        list_a_node *actual;
};

void list_s_e_init(T_list_s_e *l);
void list_s_e_add(T_list_s_e *l, T_s_e *a);
void list_s_e_first(T_list_s_e *l);
void list_s_e_next(T_list_s_e *l);
T_s_e *list_s_e_get(T_list_s_e *l);
unsigned int list_s_e_size(T_list_s_e *l);
int list_s_e_eol(T_list_s_e *l);
T_s_e *list_s_e_remove(T_list_s_e *l);
void list_s_e_clean(T_list_s_e *l);

#endif
