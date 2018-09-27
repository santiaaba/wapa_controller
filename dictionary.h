#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef DICTIONARY_H
#define DICTIONARY_H
typedef struct dictionary T_dictionary;

/*****************************
 *         Lista de Workers
 ******************************/
typedef struct d_node{
	char *key;
	char *value;
        struct d_node *next;
} dictionary_node;

struct dictionary{
        unsigned int size;
        dictionary_node *first;
        dictionary_node *last;
};

void dictionary_init(T_dictionary *d);
int dictionary_add(T_dictionary *d, char *key, char *value);
void dictionary_remove(T_dictionary *d, char *key);
char *dictionary_get(T_dictionary *d, char *key);
void dictionary_destroy(T_dictionary **d);
void dictionary_print(T_dictionary *d);
#endif
