#include <stdio.h>
#include <pthread.h>
#include "parce.h"
#include "structs.h"
#include "json.h"

#ifndef JOB_H
#define JOB_H

#define TOKEN_SIZE	25
#define JOBID_SIZE	25

typedef enum {	
		J_GET_SITES,
		J_GET_SITE,
		J_GET_WORKERS,
		J_GET_WORKER,
		J_ADD_SITE,
		J_DEL_SITE,
		J_MOD_SITE,
		J_STOP_WORKER,
		J_START_WORKER,
		J_STOP_SITE,
		J_START_SITE
} T_job_type;

typedef enum {
		J_WHAITINA,
		J_RUNNING,
		J_DONE
} T_job_status;

typedef enum {
		J_ADMIN,
		J_TENNANT
} T_job_user;

typedef struct list_job T_list_job;

typedef char T_jobid[JOBID_SIZE];
typedef char T_jobtoken[TOKEN_SIZE];

/*****************************
		Job
******************************/
typedef struct {
        T_jobid id;
        T_jobtoken *token;
	T_job_user user;		//determina el usuario que es y por las acciones permitidas
	T_job_type type;		//tipo de accion a realizar
	T_job_status status;		//estado del job
	char *data;			//datos para realizar la accion
	char *result;			//resultado en formato json para retornar.
	unsigned int result_size;	//datos para realizar la accion
} T_job;

void job_init(T_job *j, T_jobtoken *token, T_job_type type, char *data);
void job_run(T_job *j, T_list_site *sites, T_list_worker *workers, T_list_proxy *proxys);
T_jobtoken *job_get_token(T_job *j);
char *job_get_id(T_job *j);

void job_get_sites(T_job *j, T_list_site *l);
void job_get_site(T_job *j, T_list_site *l);
void job_get_workers(T_job *j, T_list_worker *l);
void job_get_worker(T_job *j, T_list_worker *l);
void job_add_site(T_job *j, T_list_site *l);
void job_del_site(T_job *j, T_list_site *l);
void job_mod_site(T_job *j, T_list_site *l);
void job_stop_worker(T_job *j, T_list_worker *l);
void job_start_worker(T_job *j, T_list_worker *l);
void job_stop_site(T_job *j, T_list_site *l);
void job_start_site(T_job *j, T_list_site *l);

/*****************************
         Lista de Jobs
******************************/
typedef struct j_node {
        T_job *data;
        struct j_node *next;
} list_j_node;

struct list_job {
        unsigned int size;
        list_j_node *first;
        list_j_node *last;
        list_j_node *actual;
};

void list_job_init(T_list_job *l);
void list_job_add(T_list_job *l, T_job *j);
void list_job_first(T_list_job *l);
void list_job_next(T_list_job *l);
T_job *list_job_get(T_list_job *l);
unsigned int list_job_size(T_list_job *l);
int list_job_eol(T_list_job *l);
T_job *list_job_remove(T_list_job *l);
void list_job_print(T_list_job *l);

#define JOB_H
#endif
