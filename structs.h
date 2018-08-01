#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


#ifndef STRUCT_H
#define STRUCT_H

#define BUFFERSIZE 512

typedef enum { S_ONLINE, S_OFFLINE} T_site_status;
typedef enum { W_ONLINE, W_OFFLINE} T_worker_status;

typedef struct list_worker T_list_worker;
typedef struct list_site T_list_site;
typedef struct list_proxy T_list_proxy;

/*****************************
 	Sitio
******************************/
typedef struct {
	unsigned long id;
	unsigned int version;
	char name[100];
	char userid[100];
	char susc[100];
	char alias[100];
	T_site_status status;
	T_list_worker *workers;
} T_site;

void site_init(T_site *s, char *name);
unsigned long int site_get_id(T_site *s);
unsigned int site_get_version(T_site *s);
T_list_worker *site_get_workers(T_site *s);
char *site_get_name(T_site *s);
char *site_get_userid(T_site *s);
char *site_get_susc(T_site *s);
char *site_get_alias(T_site *s);
T_site_status site_get_status(T_site *s);

/*****************************
 	Worker
******************************/
typedef struct {
	char name[100];
	char ip[15];
	T_worker_status status;
	T_list_site *sites;
	struct sockaddr_in socket;
} T_worker;

void worker_init(T_worker *w, char *name, char *ip);
char *worker_get_name(T_worker *w);
char *worker_get_ip(T_worker *w);
int worker_add_site(T_worker *w, T_site *s);
int worker_connect(T_worker *w);
int worker_send_recive(T_worker *w, char *command, char *buffer_recv);
int worker_sync(T_worker *w);
T_worker_status worker_get_status(T_worker *w);

/*****************************
 	Proxy
******************************/
typedef struct {
	char name[100];
	char ip[15];
} T_proxy;

void proxy_init(T_proxy *p, char *name, char *ip);
char *proxy_get_name(T_proxy *p);
char *proxy_get_ip(T_proxy *p);

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
};

void list_worker_init(T_list_worker *l);
void list_worker_add(T_list_worker *l, T_worker *w);
T_worker *list_worker_get(T_list_worker *l);
unsigned int list_worker_size(T_list_worker *l);
int list_worker_eol(T_list_worker *l);
void list_worker_remove(T_list_worker *l);
void list_worker_destroy(T_list_worker *l);

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
};

void list_site_init(T_list_site *l);
void list_site_add(T_list_site *l, T_site *s);
T_site *list_site_get(T_list_site *l);
unsigned int list_site_size(T_list_site *l);
int list_site_eol(T_list_site *l);
void list_site_remove(T_list_site *l);
void list_site_destroy(T_list_site *l);
T_site *list_site_find_site_id(T_list_site *l, unsigned long site_id);

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
};

void list_proxy_init(T_list_proxy *l);
void list_proxy_add(T_list_proxy *l, T_proxy *s);
T_proxy *list_proxy_get(T_list_proxy *l);
unsigned int list_proxy_size(T_list_proxy *l);
int list_proxy_eol(T_list_proxy *l);
void list_proxy_remove(T_list_proxy *l);
void list_proxy_destroy(T_list_proxy *l);

#endif
