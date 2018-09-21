#include <stdio.h>
#include <pthread.h>
#include "parce.h"
#include "structs.h"
#include "json.h"
#include "dictionary.h"
#include "db.h"
#include <time.h>

#ifndef JOB_H
#define JOB_H

#define TOKEN_SIZE		25
#define TASKID_SIZE		25
#define TASKRESULT_SIZE		200

typedef enum {	
		T_TASK_SHOW,

		T_SITE_LIST,
		T_SITE_SHOW,
		T_SITE_ADD,
		T_SITE_MOD,
		T_SITE_DEL,
		T_SITE_STOP,
		T_SITE_START,

		T_SERVER_LIST,
		T_SERVER_SHOW,
		T_SERVER_ADD,
		T_SERVER_MOD,
		T_SERVER_DEL,
		T_SERVER_STOP,
		T_SERVER_START
} T_task_type;

typedef enum {
		T_WHAITINA,
		T_RUNNING,
		T_DONE
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
	T_task_status status;		//estado del task
	T_dictionary *data;		//datos necesarios para realizar la accion
	char *result;			//resultado a retornar
	unsigned int result_size;	//datos para realizar la accion
} T_task;

T_task_type task_c_to_type(char c);

void task_init(T_task *t, T_task_type type, T_dictionary *data);
void task_destroy(T_task **t);
void task_run(T_task *t, T_list_site *sites, T_list_worker *workers, T_list_proxy *proxys,T_db *db);
char *task_get_token(T_task *t);
char *task_get_id(T_task *t);
char *task_get_result(T_task *t);

void task_show(T_task *t);

void task_site_list(T_task *t, T_db *db);
void task_site_show(T_task *t, T_db *db);
int task_site_add(T_task *t, T_list_site *l, T_db *db);
int task_site_del(T_task *t, T_list_site *l, T_db *db);
int task_site_mod(T_task *t, T_list_site *l, T_db *db);
int task_site_stop(T_task *t, T_list_site *l, T_db *db);
int task_site_start(T_task *t, T_list_site *l, T_db *db);

void task_worker_list(T_task *t, T_list_worker *l);
void task_worker_show(T_task *t, T_list_worker *l);
int task_worker_stop(T_task *t, T_list_worker *l, T_db *db);
int task_worker_start(T_task *t, T_list_worker *l, T_db *db);


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
int  heap_task_exist(T_heap_task *h,T_taskid id);
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
void bag_task_print(T_bag_task *b);

#define JOB_H
#endif
