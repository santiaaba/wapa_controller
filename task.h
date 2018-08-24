#include <stdio.h>
#include <pthread.h>
#include "parce.h"
#include "structs.h"
#include "json.h"
#include "dictionary.h"
#include "db.h"

#ifndef JOB_H
#define JOB_H

#define TOKEN_SIZE		25
#define TASKID_SIZE		25
#define TASKRESULT_SIZE		200

typedef enum {	
		T_GET_SITES,
		T_GET_SITE,
		T_GET_WORKERS,
		T_GET_WORKER,
		T_ADD_SITE,
		T_DEL_SITE,
		T_MOD_SITE,
		T_STOP_WORKER,
		T_START_WORKER,
		T_STOP_SITE,
		T_START_SITE
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
        T_tasktoken *token;
	T_task_user user;		//determina el usuario que es y por las acciones permitidas
	T_task_type type;		//tipo de accion a realizar
	T_task_status status;		//estado del task
	T_dictionary *data;		//datos necesarios para realizar la accion
	char *result;			//resultado en formato json para retornar.
	unsigned int result_size;	//datos para realizar la accion
} T_task;

void task_init(T_task *t, T_tasktoken *token, T_task_type type, T_dictionary *data);
void task_destroy(T_task **t);
void task_run(T_task *t, T_list_site *sites, T_list_worker *workers,
	T_list_proxy *proxys,T_db *db);
T_tasktoken *task_get_token(T_task *t);
char *task_get_id(T_task *t);
char *task_get_result(T_task *t);

void task_get_sites(T_task *t, T_list_site *l);
void task_get_site(T_task *t, T_list_site *l);
void task_get_workers(T_task *t, T_list_worker *l);
void task_get_worker(T_task *t, T_list_worker *l);
int task_add_site(T_task *t, T_list_site *l, T_db *db);
int task_del_site(T_task *t, T_list_site *l, T_db *db);
int task_mod_site(T_task *t, T_list_site *l, T_db *db);
int task_stop_worker(T_task *t, T_list_worker *l, T_db *db);
int task_start_worker(T_task *t, T_list_worker *l, T_db *db);
int task_stop_site(T_task *t, T_list_site *l, T_db *db);
int task_start_site(T_task *t, T_list_site *l, T_db *db);

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
