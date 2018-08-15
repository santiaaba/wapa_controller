#include "job.h"

void random_job_id(T_jobid value){
	/*Genera un string random para job_id */
	char *string = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	int i,j;

	for(j=0;j<JOBID_SIZE;j++){
		i = rand() % 62;
		value[j] = string[i];
	}
}

void random_token(T_jobtoken value){
	/*Genera un string random para token_id */
	char *string = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	int i,j;

	for(j=0;j<TOKEN_SIZE;j++){
		i = rand() % 62;
		value[j] = string[i];
	}
}

/*****************************
	      Job
******************************/

void job_init(T_job *j, T_jobtoken *token, T_job_type type, char *data){
	random_job_id(j->id);
	j->token = token;
	j->type = type;
	j->result = (char *)malloc(100);;
	j->result_size = 100;
}

void job_get_sites(T_job *j, T_list_site *l){
	json_sites(j->result,&(j->result_size),l);
}

void job_get_site(T_job *j, T_list_site *l){
	//j->data debe ser un id valido de sitio
	json_site(j->data,&(j->result_size),list_site_find_id(l,atoi(j->data)));
}

void job_get_workers(T_job *j, T_list_worker *l){
}

void job_get_worker(T_job *j, T_list_worker *l){
}

T_jobtoken *job_get_token(T_job *j){
	return j->token;
}

char *job_get_id(T_job *j){
	return j->id;
}

void job_add_site(T_job *j, T_list_site *l){
}

void job_del_site(T_job *j, T_list_site *l){
}

void job_mod_site(T_job *j, T_list_site *l){
}

void job_stop_worker(T_job *j, T_list_worker *l){
}

void job_start_worker(T_job *j, T_list_worker *l){
}

void job_stop_site(T_job *j, T_list_site *l){
}

void job_start_site(T_job *j, T_list_site *l){
}

void job_run(T_job *j, T_list_site *sites, T_list_worker *workers, T_list_proxy *proxys){
	/* Ejecuta el JOB */
	j->status = J_RUNNING;

	switch(j->type){
		case J_GET_SITES:
			job_get_sites(j,sites); break;
		case J_GET_SITE:
			job_get_site(j,sites); break;
		case J_GET_WORKERS:
			job_get_workers(j,workers); break;
		case J_GET_WORKER:
			job_get_worker(j,workers); break;
		case J_ADD_SITE:
			job_add_site(j,sites); break;
		case J_DEL_SITE:
			job_del_site(j,sites); break;
		case J_MOD_SITE:
			job_mod_site(j,sites); break;
		case J_STOP_WORKER:
			job_stop_worker(j,workers); break;
		case J_START_WORKER:
			job_start_worker(j,workers); break;
		case J_STOP_SITE:
			job_stop_site(j,sites); break;
		case J_START_SITE:
			job_start_site(j,sites); break;
	}
	j->status = J_DONE;
}

/*****************************
	  Lista de Jobs
 ******************************/

void list_job_init(T_list_job *l){
	l->first = NULL;
	l->actual = NULL;
	l->last = NULL;
	l->size = 0;
}

void list_job_add(T_list_job *l, T_job *j){
	list_j_node *new;
	list_j_node *aux;

	printf("Entroooo\n");
	new = (list_j_node*)malloc(sizeof(list_j_node));
	new->next = NULL;
	new->data = j;
	l->size++;

	if(l->first == NULL){
		l->first = new;
		l->last = new;
	} else {
		l->last->next = new;
		l->last = new;
	}
}

void list_job_first(T_list_job *l){
	l->actual = l->first;
}

void list_job_next(T_list_job *l){
	if(l->actual != NULL){
		l->actual = l->actual->next;
	}
}

T_job *list_job_get(T_list_job *l){
	return l->actual->data;
}

unsigned int list_job_size(T_list_job *l){
	return l->size;
}

int list_job_eol(T_list_job *l){
	return (l->actual == NULL);
}

T_job *list_job_remove(T_list_job *l){

	list_j_node *prio;
	list_j_node *aux;
	T_job *element;

	if(l->actual != NULL){
		aux = l->first;
		prio = NULL;
		while(aux != l->actual){
			prio = aux;
			aux = aux->next;
		}
		if(prio == NULL){
			l->first = aux->next;
		} else {
			prio->next = aux->next;
		}
		if(aux == l->last){
			l->last = prio;
		}
		l->actual = aux->next;
		element = aux->data;
		free(aux);
		l->size--;
	}
	return element;
}

void list_job_print(T_list_job *l){
	list_job_first(l);
	while(!list_job_eol(l)){
		printf("Job_ID: %s\n",job_get_id(list_job_get(l)));
		list_job_next(l);
	}
}
