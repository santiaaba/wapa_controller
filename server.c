#include "server.h"

void server_add_task(T_server *s, T_task *t){
	printf("Agregamos el JOB: %s a la lista\n",task_get_id(t));

	pthread_mutex_lock(&(s->mutex_heap_task));
		printf("ADD_TASK - %s\n",task_get_id(t));
		heap_task_push(&(s->tasks_todo),t);
	pthread_mutex_unlock(&(s->mutex_heap_task));

}

void *server_do_task(void *param){
	T_task *task;
	T_server *s= (T_server *)param;

	while(1){
		sleep(3);
		pthread_mutex_lock(&(s->mutex_heap_task));
			task = heap_task_pop(&(s->tasks_todo));
			printf("DO_TASK - %s\n",task_get_id(task));
		pthread_mutex_unlock(&(s->mutex_heap_task));
		if(task != NULL){
			pthread_mutex_lock(&(s->mutex_lists));
				task_run(task,s->sites,s->workers,s->proxys,s->db);
			pthread_mutex_unlock(&(s->mutex_lists));
			pthread_mutex_lock(&(s->mutex_bag_task));
				printf("BAG_TASK - %s\n",task_get_id(task));
				bag_task_add(&(s->tasks_done),task);
				bag_task_print(&(s->tasks_done));
			pthread_mutex_unlock(&(s->mutex_bag_task));
		}
	}
}

void server_get_task(T_server *s, T_taskid *taskid, char **result, unsigned int *size){
	T_task *task;
	unsigned int total_size;

	pthread_mutex_lock(&(s->mutex_bag_task));
	pthread_mutex_lock(&(s->mutex_heap_task));
		printf("Buscadno TASK ID: %s\n",taskid);
		/* Buscamos en la bolsa de tareas finalizadas */
		bag_task_print(&(s->tasks_done));
		task = bag_task_pop(&(s->tasks_done),taskid);
		if(NULL == task){
			/* Verificamos si esta en la cola de tareas pendientes */
			if(heap_task_exist(&(s->tasks_todo),(char *)taskid)){
				/* Tarea existe y esta en espera */
				sprintf(*result,"{\"taskid\":\"%s\",\"status\":\"WHAIT\"}",taskid);
			} else {
				/* Tarea no existe mas */
				sprintf(*result,"{\"taskid\":\"%s\",\"status\":\"INEXIST\"}",taskid);
			}
		} else {
			/* Tarea existe y ha finalizado */
			sprintf(*result,"{\"taskid\":\"%s\",\"status\":\"DONE\",\"result\":\"",taskid);
			total_size = (strlen(*result) + strlen(task_get_result(task)));
			if(total_size > *size){
				*result = (char *)realloc(*result,total_size + 10);
				*size = total_size + 10;
			}
			strcat(*result,task_get_result(task));
			strcat(*result,"\"}");
			/* Eliminamos el task */
			printf("ELIMINAMOS EL TASK\n");
			task_destroy(&task);
			printf("TASK ELIMINADO\n");
		}
	pthread_mutex_unlock(&(s->mutex_heap_task));
	pthread_mutex_unlock(&(s->mutex_bag_task));
}

void buffer_to_dictionary(char *buffer_rx, T_dictionary *data){
	int pos=2;
	int largo;
	char name[100];
	char value[100];

	largo = strlen(buffer_rx);
	while(pos<largo){
		parce_data(buffer_rx,'|',&pos,name);
		parce_data(buffer_rx,'|',&pos,value);
		dictionary_add(data,name,value);
	}
}

int create_task(T_task **task, char *buffer_rx){
	int pos=0;
	char value[100];
	char command;
	T_dictionary *data;

	*task=(T_task *)malloc(sizeof(T_task));
	data=(T_dictionary *)malloc(sizeof(T_dictionary));
	dictionary_init(data);
	buffer_to_dictionary(buffer_rx,data);
	parce_data(buffer_rx,'|',&pos,value);
	task_init(*task,task_c_to_type(value[0]),data);
	return 1;
}

void *server_listen(void *param){
	struct sockaddr_in server;
	struct sockaddr_in client;
	int fd_server;
	int fd_client;
	int sin_size;
	char buffer_rx[BUFFER_SIZE];
	char buffer_tx[BUFFER_SIZE];
	T_task *task;
	T_server *s= (T_server *)param;

	if ((fd_server=socket(AF_INET, SOCK_STREAM, 0)) == -1 ) {
		printf("error en socket()\n");
		exit(1);
	}
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = INADDR_ANY;

	if(bind(fd_server,(struct sockaddr*)&server, sizeof(struct sockaddr))<0) {
		printf("error en bind() \n");
		exit(1);
	}

	if(listen(fd_server,BACKLOG) == -1) {  /* llamada a listen() */
		printf("error en listen()\n");
		exit(1);
	}
	sin_size=sizeof(struct sockaddr_in);

	while(1){
		printf("Esperando conneccion desde el cliente()\n"); //Debemos mantener viva la conexion
		if ((fd_client = accept(fd_server,(struct sockaddr *)&client,&sin_size))<0) {
			printf("error en accept()\n");
			exit(1);
		}

		// Aguardamos continuamente que el cliente envie un comando
		while(recv(fd_client,buffer_rx,BUFFER_SIZE,0)>0){
			printf("Recibimos -%s-\n",buffer_rx);
			/* Creamos el task */
			if(create_task(&task,buffer_rx)){
				server_add_task(s,task);
			}
		}
		close(fd_client);
	}
}

void server_init(T_server *s, T_list_site *sites, T_list_worker *workers,
	T_list_proxy *proxys, T_db *db){

	s->sites = sites;
	s->proxys = proxys;
	s->workers = workers;
	s->db = db;
	heap_task_init(&(s->tasks_todo));
	bag_task_init(&(s->tasks_done));
	if(0 != pthread_create(&(s->thread), NULL, &server_listen, s)){
		printf ("Imposible levantar el servidor\n");
		exit(2);
	}
	if(0 != pthread_create(&(s->do_task), NULL, &server_do_task, s)){
		printf ("Imposible levantar el hilo para realizar tareas\n");
		exit(2);
	}
}

void server_lock(T_server *s){
	/* seccion critica manejo de listas */
	pthread_mutex_lock(&(s->mutex_lists));
}

void server_unlock(T_server *s){
	/* seccion critica manejo de listas */
	pthread_mutex_unlock(&(s->mutex_lists));
}
