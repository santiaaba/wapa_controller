#include <stdio.h>
#include <pthread.h>
#include "parce.h"
#include "config.h"
#include "structs.h"
#include "json.h"
#include "lista.h"
#include "dictionary.h"
#include "db.h"
#include "logs.h"
#include "dim_string.h"
#include <time.h>

#ifndef TASK_H
#define TASK_H

#define TOKEN_SIZE		25
#define TASKID_SIZE		25
#define TASKRESULT_SIZE		200

#define SYSTEM_DO	\
	if(system(command) != 0){ \
		task_done(t,HTTP_500,"ERROR FATAL"); \
		return 0; \
	}

#define		M_DB_ERROR			"Error contra la base de datos"
#define		M_LOGIN_ERROR		"Error de login"
#define		M_SITE_LIMT_ERROR	"Se supera el limite de sitios para el namespace"
#define		M_FTP_LIMT_ERROR	"Se supera el limite de usuarios ftp por sitio"
#define		M_TIMEOUT_ERROR		"tarea vencio por timeout"


#define		HTTP_200	200		/* OK */

#define		HTTP_400	400		/* Bad request */
#define		HTTP_401	401		/* Credenciales no validas */
#define		HTTP_403	403		/* Forbidden */
#define		HTTP_404	404		/* Not found */

#define		HTTP_410	410		/* Superamos el limite de sitios para el namespace */
#define		HTTP_411	411		/* Error al agregar sitio */

#define		HTTP_420	420		/* Superamos el limite de usuarios ftp por sitio */

#define		HTTP_499	499		/* Error aun no clasificado */

#define		HTTP_500	500		/* Error faltal */
#define		HTTP_501	501		/* Error contra la base de datos */
#define		HTTP_502	502		/* task time out */

typedef enum {	
		T_TASK_NONE,			// 0
		T_LOGIN,
		T_TASK_SHOW,

		T_NAMESPACE_LIST,
		T_NAMESPACE_SHOW,
		T_NAMESPACE_ADD,
		T_NAMESPACE_DEL,
		T_NAMESPACE_MOD,		// 7
		T_NAMESPACE_STOP,
		T_NAMESPACE_START,

		T_SITE_LIST,
		T_SITE_LIST_ALL,
		T_SITE_SHOW,
		T_SITE_ADD,				// 13
		T_SITE_MOD,
		T_SITE_DEL,
		T_SITE_ALLDEL,
		T_SITE_STOP,			// 17
		T_SITE_START,

		T_SERVER_LIST,
		T_SERVER_SHOW,
		T_SERVER_ADD,
		T_SERVER_MOD,
		T_SERVER_DEL,
		T_SERVER_STOP,
		T_SERVER_START,

		T_FTP_LIST,
		T_FTP_ADD,
		T_FTP_DEL,
		T_FTP_MOD
} T_task_type;

typedef enum { T_WAITING,
		T_RUNNING,
		T_DONE,
		T_TODO,
} T_task_status;

typedef enum {
		T_ADMIN,
		T_TENNANT
} T_task_user;

typedef struct heap_task T_heap_task;
typedef struct bag_task T_bag_task;

typedef char T_taskid[TASKID_SIZE];
typedef char T_tasktoken[TOKEN_SIZE];

/*****************************
		TASK	
******************************/
typedef struct {
    T_taskid id;
    T_tasktoken token;
	T_task_user user;		//determina el usuario que es y por las acciones permitidas
	T_task_type type;		//tipo de accion a realizar
	T_task_status status;	//estado del task
	T_dictionary *data;		//datos necesarios para realizar la accion
	time_t time;			//Instante de tiempo en que la tarea ingresa o finaliza
	char *result;			//resultado a retornar
	int http_code;			// Codigo http de rest a retornar
} T_task;

int valid_task_id(char *s);
void task_init(T_task *t);
void task_set(T_task *t, T_task_type type, T_dictionary *data);
time_t task_get_time(T_task *t);
void task_destroy(T_task **t);
void task_run(T_task *t, T_lista *sites, T_lista *workers,
	      	  T_lista *proxys,T_db *db, T_config *config, T_logs *logs);
void task_done(T_task *t, int http_code, char *message);
T_dictionary *task_get_data(T_task *t);
char *task_get_token(T_task *t);
char *task_get_id(T_task *t);
char *task_get_result(T_task *t);
T_task_status task_get_status(T_task *t);
void task_show(T_task *t);

/*****************************
         Cola de tareas
******************************/
typedef struct j_h_node {
        T_task *data;
        struct j_h_node *next;
} heap_t_node;

struct heap_task {
        unsigned int size;
        heap_t_node *first;
        heap_t_node *last;
};

void heap_task_init(T_heap_task *h);
void heap_task_push(T_heap_task *h, T_task *t);
T_task *heap_task_exist(T_heap_task *h,T_taskid id);
T_task *heap_task_pop(T_heap_task *h);
unsigned int heap_task_size(T_heap_task *h);
void heap_task_print(T_heap_task *h);

/****************************
 	BAG Jobs
*****************************/

typedef struct j_b_node {
        T_task *data;
        struct j_b_node *next;
} bag_t_node;

struct bag_task {
        unsigned int size;
        bag_t_node *first;
        bag_t_node *last;
        bag_t_node *actual;
};

void bag_task_init(T_bag_task *b);
void bag_task_add(T_bag_task *b, T_task *t);
T_task *bag_task_pop(T_bag_task *b, T_taskid *id);
unsigned int bag_task_size(T_bag_task *b);
void bag_task_timedout(T_bag_task *b, int d);
void bag_task_print(T_bag_task *b);

#define JOB_H
#endif
