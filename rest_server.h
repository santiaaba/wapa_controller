#include <microhttpd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "json.h"
#include "dictionary.h"
#include "task.h"

#define PORT		8888
#define POSTBUFFERSIZE  512
#define MAXNAMESIZE     20
#define MAXANSWERSIZE   512
#define GET		0
#define POST		1
#define DELETE		2

typedef struct t_r_server {
	T_heap_task tasks_todo;
	T_bag_task tasks_done;
	struct MHD_Daemon *rest_daemon;
	pthread_t thread;
	pthread_t do_task;
	pthread_mutex_t mutex_heap_task;
	pthread_mutex_t mutex_bag_task;
	pthread_mutex_t mutex_lists;
	T_list_site *sites;
        T_list_worker *workers;
        T_list_proxy *proxys;
	T_db *db;
	} T_rest_server;

struct connection_info_struct {
	int connectiontype;
	T_dictionary *data;
	//char *answerstring;
	struct MHD_PostProcessor *postprocessor;
};

/* Variables externas que se encuentran en controller.c,
 * el cual utiliza esta libreria. Guarda que estas variables
 * se encuentran en LA ZONA CRITICA de los hilos */

//extern T_list_site sites;
//extern T_list_worker workers;
//extern T_list_proxy proxys;
extern T_rest_server rest_server;

void rest_server_init(T_rest_server *r, T_list_site *sites,
			T_list_worker *workers, T_list_proxy *proxys, T_db *db);
void rest_server_add_task(T_rest_server *r, T_task *j);

void rest_server_lock(T_rest_server *r);

void rest_server_unlock(T_rest_server *r);
