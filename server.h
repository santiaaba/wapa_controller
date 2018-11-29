#include <stdint.h>
#include <microhttpd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "json.h"
#include "dictionary.h"
#include "task.h"
#include "config.h"

#define PORT		8888
#define MAXNAMESIZE     20
#define MAXANSWERSIZE   512
#define BUFFER_SIZE	25
#define HEADER_SIZE	8
#define BACKLOG 	1 /* El número de conexiones permitidas */

#define CHECK_VALID_ID(X,Y)     if(!valid_id(dictionary_get(d,#X))){ \
                                        sprintf(message,"300|\"code\":\"300\",\"info\":\"El %s id=%s es invalido\"", \
					#Y,dictionary_get(d,#X)); \
                                        return 0; \
                                }

#define CHECK_VALID_U_P		if(!valid_user_name(dictionary_get(d,"name"))){ \
			                sprintf(message,"300|\"code\":\"300\",\"info\":\"Nombre usuario invalido\""); \
					return 0; \
				} \
				if(!valid_passwd(dictionary_get(d,"passwd"))){ \
					sprintf(message,"300|\"code\":\"300\",\"info\":\"passwd invalida\""); \
					return 0; \
				}

typedef struct t_r_server {
	T_heap_task tasks_todo;
	T_bag_task tasks_done;
	pthread_t thread;
	pthread_t do_task;
	pthread_mutex_t mutex_heap_task;
	pthread_mutex_t mutex_bag_task;
	pthread_mutex_t mutex_lists;
	T_list_site *sites;
        T_list_worker *workers;
        T_list_proxy *proxys;
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

void server_init(T_server *s, T_list_site *sites, T_list_worker *workers,
		 T_list_proxy *proxys, T_db *db, T_config *config, T_logs *logs);
void server_lock(T_server *s);
void server_unlock(T_server *s);
