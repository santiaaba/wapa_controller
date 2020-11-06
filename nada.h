#ifndef VSTRING_H
#define VSTRING_H

/**********************************
 * 	lista strings
 **********************************/
typedef struct list_string T_list_string;

typedef struct v_node {
        char *data;
        struct v_node *next;
} list_v_node;

struct list_string{
        unsigned int size;
        list_v_node *first;
        list_v_node *last;
        list_v_node *actual;
};

void list_string_init(T_list_string *l);
void list_string_add(T_list_string *l, char *s);
char *list_string_get(T_list_string *l);
void list_string_first(T_list_string *l);
void list_string_next(T_list_string *l);
unsigned int list_string_size(T_list_string *l);
int list_string_eol(T_list_string *l);
char *list_string_remove(T_list_string *l);

#endif
