#include <stdint.h>
#include <microhttpd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "json.h"
#include "dictionary.h"
#include "task.h"
#include "config.h"

#define REST_PORT		8888
#define POSTBUFFERSIZE	1024
#define MAXNAMESIZE		20
#define MAXANSWERSIZE	512
#define GET				0
#define POST			1
#define PUT				2
#define DELETE			3

#define CHECK_VALID_ID(X,Y)     \
	if(!valid_id(dictionary_get(d,#X))){ \
		sprintf(message,"El %s id=%s es invalido", \
		#Y,dictionary_get(d,#X)); \
		return 0; \
	}

#define CHECK_VALID_U_P		\
	if(!valid_user_name(dictionary_get(d,"name"))){ \
		sprintf(message,"Nombre usuario invalido"); \
		return 0; \
	} \
	if(!valid_passwd(dictionary_get(d,"passwd"))){ \
		sprintf(message,"Passwd invalida"); \
		return 0; \
	}

#define CHECK_VALID_NAME(X,Y)     \
	if(!valid_namespace_name(dictionary_get(d,#X))){ \
		sprintf(message,"El nombre del namespace es invalido"); \
		return 0; \
	}

#define CHECK_VALID_SITE_NAME(X)	\
	if(!valid_site_name(dictionary_get(d,#X))){ \
		sprintf(message,"El nombre del sitio es invalido"); \
		return 0; \
	}

struct connection_info_struct {
	int connectiontype;
	T_dictionary *data;
	struct MHD_PostProcessor *postprocessor;
};

typedef struct t_r_server {
	T_heap_task tasks_todo;
	T_task *runningTask;
	T_bag_task tasks_done;
	struct MHD_Daemon *rest_daemon;
	pthread_t thread;
	pthread_t do_task;
	pthread_t purge_done;
	pthread_mutex_t mutex_heap_task;
	pthread_mutex_t mutex_bag_task;
	pthread_mutex_t mutex_lists;
	T_lista *sites;
	T_lista *workers;
	T_lista *proxys;
	T_logs *logs;
	T_config *config;
	T_db *db;
	struct sockaddr_in server;
	struct sockaddr_in client;
	int fd_server;
	int fd_client;
	int sin_size;
} T_server;

extern T_server server;

void server_init(T_server *s, T_lista *sites, T_lista *workers,
		 T_lista *proxys, T_db *db, T_config *config, T_logs *logs);
void server_lock(T_server *s);
void server_unlock(T_server *s);
