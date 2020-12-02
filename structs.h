#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "parce.h"
#include "dim_string.h"
#include "lista.h"
#include "sock_connect.h"

#ifndef STRUCT_H
#define STRUCT_H


#define	SITE_MAX_SIZE	10
#define ROLE_BUFFER_SIZE 25
#define ROLE_HEADER_SIZE 8
#define TIMEONLINE 20	//tiempo que debe estar en preparado para pasar a online. En segundos

typedef enum { S_OFFLINE, S_ONLINE} T_site_status;
typedef enum {SC_OK, SC_POOR, SC_NONE} T_sc_status;	/* Estado del sitio dentro del cluster */

typedef enum { W_ONLINE, W_OFFLINE, W_PREPARED, W_BROKEN, W_UNKNOWN} T_worker_status;
typedef enum { P_ONLINE, P_OFFLINE, P_PREPARED, P_BROKEN, P_UNKNOWN} T_proxy_status;

/*****************************
          Varios
******************************/
void itosstatus(T_site_status i, char *name);
void itoscstatus(T_sc_status i, char *name);
void itowstatus(T_worker_status i, char *name);
void itopstatus(T_proxy_status i, char *name);

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
int s_e_get_id(T_s_e *a);
void s_e_to_json(T_s_e *a, char **message);

/*****************************
 	Sitio
******************************/
typedef struct {
	uint32_t id;
	uint16_t version;
	char name[100];
	unsigned int size;   //4 bytes
	char *dir;   //Directorio desde los hash de los subdirectorios
				 // EJ: 06/90/a12k3123n1k231
	uint32_t namespaceId;
	char *namespaceName;
	T_site_status status;
	T_sc_status sc_status;
	T_lista *alias;
	T_lista *indexes;
	T_lista *workers;
} T_site;

void site_init(T_site *s, char *name, uint32_t id, char *dir,
               unsigned int version, unsigned int size,
			   T_site_status status, uint32_t namespaceId,
			   char *namespaceName);
uint32_t site_get_id(T_site *s);
unsigned int site_get_version(T_site *s);
void site_increse_version(T_site *s);
char *site_get_name(T_site *s);
char *site_get_dir(T_site *s);
unsigned int site_get_size(T_site *s);
unsigned int site_get_real_size(T_site *s);
T_lista *site_get_alias(T_site *s);
void site_put_alias(T_site *s, T_lista *l);
void site_put_indexes(T_site *s, T_lista *l);
T_lista *site_get_indexes(T_site *s);
T_site_status site_get_status(T_site *s);
void site_update(T_site *s);
void site_stop(T_site *s);
void site_start(T_site *s);

void site_set_size(T_site *s, unsigned int size);
void site_set_status(T_site *s, T_site_status status);

/* Retorna la lista de workers */
T_lista *site_get_workers(T_site *s);
void site_set_size(T_site *s, unsigned int size);

void site_to_json(T_site *s,char **message);

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
	T_lista *sites;
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
T_lista *worker_get_sites(T_worker *w);
float worker_num_sites(T_worker *w);

/* Agrega un sitio al worker. Esto implica agregarlo tambien
 * al worker fisico */
int worker_add_site(T_worker *w, T_site *s);

int worker_remove_site(T_worker *w, T_site *s);

/* Elimina logicamente los sitios de un worker.*/
void worker_purge(T_worker *w);

int worker_send_receive(T_worker *w, char *send_message, uint32_t send_message_size,
                        char **rcv_message, uint32_t *rcv_message_size);
int worker_sync(T_worker *w, T_lista *s);
T_worker_status worker_get_status(T_worker *w);

void worker_to_json(T_worker *w,char **message);


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
char *proxy_get_ipv4(T_proxy *p);
float proxy_get_load(T_proxy *p);
void proxy_start(T_proxy *p);
void proxy_stop(T_proxy *p);
int proxy_reload(T_proxy *p);
void proxy_reconfig(T_proxy *p, T_lista *sites);

#endif
