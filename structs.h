#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "string_element.h"
#include "parce.h"

#ifndef STRUCT_H
#define STRUCT_H

#define	SITE_MAX_SIZE	10
#define ROLE_BUFFER_SIZE 25
#define ROLE_HEADER_SIZE 8
#define TIMEONLINE 20	//tiempo que debe estar en preparado para pasar a online. En segundos

typedef enum { S_OFFLINE, S_ONLINE} T_site_status;
typedef enum { W_ONLINE, W_OFFLINE, W_PREPARED, W_BROKEN, W_UNKNOWN} T_worker_status;
typedef enum { P_ONLINE, P_OFFLINE, P_PREPARED, P_BROKEN, P_UNKNOWN} T_proxy_status;

typedef struct list_worker T_list_worker;
typedef struct list_site T_list_site;
typedef struct list_proxy T_list_proxy;

/*****************************
          Varios
******************************/
void itowstatus(T_worker_status i, char *name);

/*****************************
 	Sitio
******************************/
typedef struct {
	unsigned int id;   //4 bytes
	unsigned int version;
	char name[100];
	unsigned int size;   //4 bytes
	char dir[5];   //Directorio
	T_list_s_e *alias;
	T_list_s_e *indexes;
	T_site_status status;
	T_list_worker *workers;
} T_site;

void site_init(T_site *s, char *name, unsigned int id, char *dir,
               unsigned int version, unsigned int size);
unsigned int site_get_id(T_site *s);
unsigned int site_get_version(T_site *s);
char *site_get_name(T_site *s);
char *site_get_dir(T_site *s);
unsigned int site_get_size(T_site *s);
unsigned int site_get_real_size(T_site *s);
T_list_s_e *site_get_alias(T_site *s);
T_list_s_e *site_get_indexes(T_site *s);
T_site_status site_get_status(T_site *s);
void site_update(T_site *s);
void site_stop(T_site *s);
void site_start(T_site *s);

void site_set_size(T_site *s, unsigned int size);
void site_set_status(T_site *s, T_site_status status);

/* Retorna la lista de workers */
T_list_worker *site_get_workers(T_site *s);
void site_set_size(T_site *s, unsigned int size);

/*****************************
 	Worker
******************************/
typedef struct {
	char name[100];
	char ip[15];
	int id;
	float laverage;
	T_worker_status status;
	T_worker_status last_status;
	unsigned long time_change_status;	//timestamp
	int is_changed;		//indica si ha cambiado el estado
				// worker_change_status lo pone en 1. worker_check lo pone en 0
	T_list_site *sites;
	struct sockaddr_in server;
	int socket;
} T_worker;

void worker_init(T_worker *w, int id, char *name, char *ip, T_worker_status s);
char *worker_get_name(T_worker *w);
int worker_get_id(T_worker *w);
void worker_set_statistics(T_worker *w, char *buffer_rx);
T_worker_status worker_get_status(T_worker *w);
T_worker_status worker_get_last_status(T_worker *w);
unsigned int worker_get_last_time(T_worker *w);
int worker_check(T_worker *w);
char *worker_get_ipv4(T_worker *w);
float worker_get_load(T_worker *w);
void worker_start(T_worker *w);
void worker_stop(T_worker *w);
int worker_reload(T_worker *w);
T_list_site *worker_get_sites(T_worker *w);

/* Agrega un sitio al worker. Esto implica agregarlo tambien
 * al worker fisico */
int worker_add_site(T_worker *w, T_site *s);

int worker_remove_site(T_worker *w, T_site *s);

/* Elimina logicamente los sitios de un worker.*/
void worker_purge(T_worker *w);

int worker_send_receive(T_worker *w, char *send_message, uint32_t send_message_size,
                        char **rcv_message, uint32_t *rcv_message_size);
int worker_sync(T_worker *w, T_list_site *s);
T_worker_status worker_get_status(T_worker *w);

/*****************************
 	Proxy
******************************/
typedef struct {
	int id;
	char name[100];
	char ip[15];
	float laverage;
	T_proxy_status status;
	T_proxy_status last_status;
	unsigned long time_change_status;	//timestamp
	int is_changed;		//indica si ha cambiado el estado
				// worker_change_status lo pone en 1. worker_check lo pone en 0
	struct sockaddr_in server;
	int socket;
} T_proxy;

void proxy_init(T_proxy *p, int id, char *name, char *ip, T_proxy_status s);
char *proxy_get_name(T_proxy *p);
int proxy_get_id(T_proxy *p);
void proxy_set_statistics(T_proxy *p, char *buffer_rx);
T_proxy_status proxy_get_status(T_proxy *p);
T_proxy_status proxy_get_last_status(T_proxy *p);
unsigned int proxy_get_last_time(T_proxy *p);
int proxy_check(T_proxy *p);
int proxy_add_site(T_proxy *p, T_site *s);
int proxy_change_site(T_proxy *p, T_site *s);
char *proxy_get_ip(T_proxy *p);
float proxy_get_load(T_proxy *p);
void proxy_set_online(T_proxy *p);
void proxy_set_offline(T_proxy *p);
int proxy_reload(T_proxy *p);
void proxy_reconfig(T_proxy *p, T_list_site *sites);

/*****************************
 	Lista de Workers
******************************/
typedef struct w_node {
	T_worker *data;
	struct w_node *next;
} list_w_node;

struct list_worker{
	unsigned int size;
	list_w_node *first;
	list_w_node *last;
	list_w_node *actual;
	pthread_mutex_t lock;
};

/* Inicializa la estructura de lista */
void list_worker_init(T_list_worker *l);

/* Agrega un elemento al final de la lista */
void list_worker_add(T_list_worker *l, T_worker *w);

/* Copia un alista en otra de workers */
void list_worker_copy(T_list_worker *l, T_list_worker *l2);

/* Retorna el elemento actualmente apuntado en la lista */
T_worker *list_worker_get(T_list_worker *l);

/* Retorna el primer elemento de la lista */
T_worker *list_worker_get_first(T_list_worker *l);

/* Retorna el ultimo elemento de la lista */
T_worker *list_worker_get_last(T_list_worker *l);

/* Coloca el punto al inicio de la lista*/
void list_worker_first(T_list_worker *l);

/* Avanza el puntero un elemento en la lista*/
void list_worker_next(T_list_worker *l);

/* Retorna la cantidad de elementos en la lista */
unsigned int list_worker_size(T_list_worker *l);

/* Indica si el puntero esta al finald e la lista */
int list_worker_eol(T_list_worker *l);

/* Remueve logicamente el elemento actualmente apuntado.
 * El puntero queda apuntado al elemento siguente */
T_worker *list_worker_remove(T_list_worker *l);

/* Elimina el worker que posee el id pasado por parametro */
T_worker *list_worker_remove_id(T_list_worker *l, int worker_id);

/* retorna el elemento solicitado por su id. NULL si no existe*/
T_worker *list_worker_find_id(T_list_worker *l, int worker_id);

/* Ordena la lista de workers por la cantidad de sitios asignados.
 * El segundo parametro es 1 desscendente, 0 ascendente */
void list_worker_sort_by_site(T_list_worker *l,int des);

/* Ordena la lista de workers por el load average reportado
 * El segundo parametro es 1 desscendente, 0 ascendente */
void list_worker_sort_by_load(T_list_worker *l,int des);

void list_worker_print(T_list_worker *l);

/*****************************
 	Lista de Sitios
******************************/
typedef struct s_node {
	T_site *data;
	struct s_node *next;
} list_s_node;

struct list_site {
	unsigned int size;
	list_s_node *first;
	list_s_node *last;
	list_s_node *actual;
	pthread_mutex_t lock;
};

/* Inicializa la estructura de lista */
void list_site_init(T_list_site *l);

/* Coloca el punto al inicio de la lista*/
void list_site_first(T_list_site *l);

/* Copia un alista en otra de sitios */
void list_site_copy(T_list_site *l, T_list_site *l2);

/* Avanza el puntero un elemento en la lista*/
void list_site_next(T_list_site *l);

/* Agrega un elemento al final de la lista */
void list_site_add(T_list_site *l, T_site *s);

/* Retorna el elemento actualmente apuntado en la lista */
T_site *list_site_get(T_list_site *l);

/* Retorna la cantidad de elementos en la lista */
unsigned int list_site_size(T_list_site *l);

/* Indica si el puntero esta al finald e la lista */
int list_site_eol(T_list_site *l);

/* Remueve logicamente el elemento actualmente apuntado.
 * El puntero queda apuntado al elemento siguente */
T_site *list_site_remove(T_list_site *l);

/* Remueve logicamente el elemento con id indicado.
 * El puntero queda apuntado al elemento siguente */
T_site *list_site_remove_id(T_list_site *l, unsigned int id);

/* retorna el elemento solicitado por su id. NULL si no existe*/
T_site *list_site_find_id(T_list_site *l, unsigned int site_id);

/* Vacia la lista sin eliminar los elementos. */
void list_site_erase(T_list_site *l);

void list_site_print(T_list_site *l);

/*****************************
 	Lista de Proxys
******************************/
typedef struct p_node {
	T_proxy *data;
	struct p_node *next;
} list_p_node;

struct list_proxy {
	unsigned int size;
	list_p_node *first;
	list_p_node *last;
	list_p_node *actual;
	pthread_mutex_t lock;
};

void list_proxy_init(T_list_proxy *l);
void list_proxy_add(T_list_proxy *l, T_proxy *s);
void list_proxy_first(T_list_proxy *l);
void list_proxy_next(T_list_proxy *l);
T_proxy *list_proxy_get(T_list_proxy *l);
unsigned int list_proxy_size(T_list_proxy *l);
int list_proxy_eol(T_list_proxy *l);
void list_proxy_remove(T_list_proxy *l);
void list_proxy_destroy(T_list_proxy *l);

#endif
