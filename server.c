#include "server.h"

void int_to_4bytes(uint32_t *i, char *_4bytes){
	memcpy(_4bytes,i,4);
}

void _4bytes_to_int(char *_4bytes, uint32_t *i){
	memcpy(i,_4bytes,4);
}

/************************
 *      CLOUD	        *
 ************************/

void server_check(T_server *s, char **send_message, int *send_message_size){
	/* DE MOMENTO RETORNA SOLO 1. La idea es que retorne el estado de la nube */
	*send_message_size=2;
	*send_message = (char *)realloc(*send_message,*send_message_size);
	strcpy(*send_message,"1");
}

void server_add_task(T_server *s, T_task *t, char **message, uint32_t *message_size){

	printf("Agregamos el task a la cola\n");
	pthread_mutex_lock(&(s->mutex_heap_task));
		heap_task_push(&(s->tasks_todo),t);
		*message=(char *)realloc(*message,TASKID_SIZE + 2);
		*message_size = TASKID_SIZE + 2;	//sumamos 2 para incluir el 1 y el \0
		sprintf(*message,"1%s",task_get_id(t));
	pthread_mutex_unlock(&(s->mutex_heap_task));
}

void *server_do_task(void *param){
	T_task *task;
	T_server *s= (T_server *)param;

	while(1){
		//sleep(5);
		//printf("Corremos el task\n");
		pthread_mutex_lock(&(s->mutex_heap_task));
			task = heap_task_pop(&(s->tasks_todo));
		pthread_mutex_unlock(&(s->mutex_heap_task));
		if(task != NULL){
			pthread_mutex_lock(&(s->mutex_lists));
				task_run(task,s->sites,s->workers,s->proxys,s->db);
			pthread_mutex_unlock(&(s->mutex_lists));
			pthread_mutex_lock(&(s->mutex_bag_task));
				bag_task_add(&(s->tasks_done),task);
				bag_task_print(&(s->tasks_done));
			pthread_mutex_unlock(&(s->mutex_bag_task));
		}
	}
}

void server_get_task(T_server *s, T_taskid *taskid, char **result_message, uint32_t *size_message){
	/* result es un puntero a NULL */
	T_task *task;
	unsigned int total_size;

	pthread_mutex_lock(&(s->mutex_bag_task));
	pthread_mutex_lock(&(s->mutex_heap_task));
		printf("Buscadno TASK ID: %s\n",taskid);
		/* Buscamos en la bolsa de tareas finalizadas */
		bag_task_print(&(s->tasks_done));
		task = bag_task_pop(&(s->tasks_done),taskid);
		if(NULL == task){
			/* No ha finalizado o no existe */
			*size_message=2;
			*result_message=(char *)realloc(*result_message,*size_message);
			/* Verificamos si esta en la cola de tareas pendientes */
			if(heap_task_exist(&(s->tasks_todo),(char *)taskid)){
				/* Tarea existe y esta en espera */
				strcpy(*result_message,"2");
			} else {
				/* Tarea no existe */
				strcpy(*result_message,"0");
			}
		} else {
			printf("TASK finalizado. Informamos\n");
			// A size_message le sumamos dos bytes. Uno para el 1 (task finalizado) y otro para el \0
			*size_message = strlen(task_get_result(task)) + 2;
			*result_message=(char *)realloc(*result_message,((*size_message)));
			sprintf(*result_message,"1%s",task_get_result(task));
			printf("RESULTADO TASK:: %i - %s\n",*size_message,*result_message);
			/* Eliminamos el task */
			task_destroy(&task);
		}
	pthread_mutex_unlock(&(s->mutex_heap_task));
	pthread_mutex_unlock(&(s->mutex_bag_task));
}

void buffer_to_dictionary(char *message, T_dictionary *data, int *pos){
	int largo;
	char name[100];
	char value[100];

	largo = strlen(message);
	while(*pos<largo){
		parce_data(message,'|',pos,name);
		parce_data(message,'|',pos,value);
		dictionary_add(data,name,value);
	}
}

int create_task(T_task **task, char *buffer_rx){
	/* Crea una tarea. Si hay un error lo especifica en
 	   la variable message */
	char value[100];
	char command;
	int pos=1;
	T_dictionary *data;

	printf("Creamos el task\n");
	*task=(T_task *)malloc(sizeof(T_task));
	data=(T_dictionary *)malloc(sizeof(T_dictionary));
	dictionary_init(data);
	buffer_to_dictionary(buffer_rx,data,&pos);
	task_init(*task,task_c_to_type(buffer_rx[0]),data);
	return 1;
}

int recv_all_message(T_server *s, char **rcv_message, uint32_t *rcv_message_size){
	/* Coordina la recepcion de mensajes grandes del cliente */
	/* El encabezado es de 1 char */
	char buffer[BUFFER_SIZE];
	char printB[BUFFER_SIZE+1];
	int first_message=1;
	uint32_t parce_size;
	int c=0;	// Cantidad de bytes recibidos

	// Al menos una vez vamos a ingresar
	*rcv_message_size=0;
	do{
		if(recv(s->fd_client,buffer,BUFFER_SIZE,0)<=0){
			return 0;
		}
		if(first_message){
			first_message=0;
			_4bytes_to_int(buffer,rcv_message_size);
			*rcv_message=(char *)realloc(*rcv_message,*rcv_message_size);
		}
		_4bytes_to_int(&(buffer[4]),&parce_size);
		memcpy(*rcv_message+c,&(buffer[HEADER_SIZE]),parce_size);
		c += parce_size;
	} while(c < *rcv_message_size);
	printf("RECV Mesanje: %s\n",*rcv_message);
	return 1;
}

int send_all_message(T_server *s, char *send_message, uint32_t send_message_size){
	/* Coordina el envio de los datos aun cuando se necesita
	 * mas de una transmision */
	char buffer[BUFFER_SIZE];
	char printB[BUFFER_SIZE+1];
	int c=0;	//Cantidad byes enviados
	uint32_t parce_size;

	printf("Enviaremos al CORE: %s\n",send_message);
	if( send_message[send_message_size-1] != '\0'){
		printf("cloud_send_receive: ERROR. send_message no termina en \\0");
		return 0;
	}

	/* Los 4 primeros bytes del header es el tamano total del mensaje */
        int_to_4bytes(&send_message_size,buffer);

	while(c < send_message_size){
		if(send_message_size - c + HEADER_SIZE < BUFFER_SIZE){
			/* Entra en un solo buffer */
			parce_size = send_message_size - c ;
		} else {
			/* No entra todo en el buffer */
			parce_size = BUFFER_SIZE - HEADER_SIZE;
		}
		int_to_4bytes(&parce_size,&(buffer[4]));
		memcpy(buffer + HEADER_SIZE,send_message + c,parce_size);
		c += parce_size;
		//printf("Enviamos %s\n",buffer[HEADER_SIZE]);
		if(send(s->fd_client,buffer,BUFFER_SIZE,0)<0){
			printf("ERROR a manejar\n");
			return 0;
		}
	}
}

void *server_listen(void *param){
	char *recv_message = NULL;
	char *send_message = NULL;
	uint32_t recv_message_size = 0;
	uint32_t send_message_size = 0;
	int pos;
	char taskid[20];
	T_task *task;
	T_server *s= (T_server *)param;

	if ((s->fd_server=socket(AF_INET, SOCK_STREAM, 0)) == -1 ) {
		printf("error en socket()\n");
		exit(1);
	}
	s->server.sin_family = AF_INET;
	s->server.sin_port = htons(PORT);
	s->server.sin_addr.s_addr = INADDR_ANY;

	if(bind(s->fd_server,(struct sockaddr*)&(s->server), sizeof(struct sockaddr))<0) {
		printf("error en bind() \n");
		exit(1);
	}

	if(listen(s->fd_server,BACKLOG) == -1) {  /* llamada a listen() */
		printf("error en listen()\n");
		exit(1);
	}
	s->sin_size=sizeof(struct sockaddr_in);

	recv_message = (char *)malloc(10);
	send_message = (char *)malloc(10);
	while(1){
		printf("Esperando conneccion desde el cliente()\n"); //Debemos mantener viva la conexion
		if ((s->fd_client = accept(s->fd_server,(struct sockaddr *)&(s->client),&(s->sin_size)))<0) {
			printf("error en accept()\n");
			exit(1);
		}

		// Aguardamos continuamente que el cliente envie un comando
		while(recv_all_message(s,&recv_message,&recv_message_size)){
			printf("Recibimos -%s-\n",recv_message);
			if(recv_message[0] == 't'){
				/* nos solicitan el estado de un task */
				pos=1;
				parce_data(recv_message,'|',&pos,taskid);
				server_get_task(s,(T_taskid *)taskid,&send_message,&send_message_size);
				printf("ENVIAMOS RESULTADO TASK %i:%i -%s-\n",send_message_size, strlen(send_message), send_message);
			} else if(recv_message[0] == 'c'){
				/* nos solicitan un chequeo */
				server_check(s,&send_message,&send_message_size);
			} else {
				/* Creamos el task  */
				if(create_task(&task,recv_message)){
					server_add_task(s,task,&send_message,&send_message_size);
				}
			}
			send_all_message(s,send_message,send_message_size);
		}
		close(s->fd_client);
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
