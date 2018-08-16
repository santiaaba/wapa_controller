#include <microhttpd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "json.h"
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
	pthread_mutex_t mutex_heap_task;
	pthread_mutex_t mutex_bag_task;
	} T_rest_server;

struct connection_info_struct {
	int connectiontype;
	char *answerstring;
	struct MHD_PostProcessor *postprocessor;
};

/* Variables externas que se encuentran en controller.c,
 * el cual utiliza esta libreria. Guarda que estas variables
 * se encuentran en LA ZONA CRITICA de los hilos */

extern T_list_site sites;
extern T_list_worker workers;
extern T_rest_server rest_server;

void rest_server_init(T_rest_server *r);
void rest_server_add_task(T_rest_server *r, T_task *j);
