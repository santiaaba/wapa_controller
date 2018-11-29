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

void server_check(T_server *s, char **send_message){
	/* DE MOMENTO RETORNA SOLO 1. La idea es que retorne el estado de la nube */
	*send_message = (char *)realloc(*send_message,2);
	strcpy(*send_message,"1");
}

void server_add_task(T_server *s, T_task *t, char **message){

	printf("Agregamos el task a la cola\n");
	pthread_mutex_lock(&(s->mutex_heap_task));
		heap_task_push(&(s->tasks_todo),t);
		*message=(char *)realloc(*message,TASKID_SIZE + 2);
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
				task_run(task,s->sites,s->workers,s->proxys,s->db,s->config,s->logs);
			pthread_mutex_unlock(&(s->mutex_lists));
			pthread_mutex_lock(&(s->mutex_bag_task));
				bag_task_add(&(s->tasks_done),task);
				bag_task_print(&(s->tasks_done));
			pthread_mutex_unlock(&(s->mutex_bag_task));
		}
	}
}

void server_get_task(T_server *s, T_taskid *taskid, char **result_message){
	/* result es un puntero a NULL */
	T_task *task;
	unsigned int total_size;
	int size_message;

	pthread_mutex_lock(&(s->mutex_bag_task));
	pthread_mutex_lock(&(s->mutex_heap_task));
		printf("Buscadno TASK ID: %s\n",taskid);
		/* Buscamos en la bolsa de tareas finalizadas */
		bag_task_print(&(s->tasks_done));
		task = bag_task_pop(&(s->tasks_done),taskid);
		if(NULL == task){
			/* No ha finalizado o no existe */
			*result_message=(char *)realloc(*result_message,2);
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
			size_message = strlen(task_get_result(task)) + 2;
			*result_message=(char *)realloc(*result_message,((size_message)));
			sprintf(*result_message,"1%s",task_get_result(task));
			printf("RESULTADO TASK:: %i - %s\n",size_message,*result_message);
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

int verify_susc_show(T_dictionary *d, char *message){
	CHECK_VALID_ID(susc_id,subscripsion)
}

int verify_susc_add(T_dictionary *d, char *message){
	/* IMPLEMENTAR */
	return 1;
}

int verify_susc_id(T_dictionary *d, char *message){
	/* Funcion generica utiliza por varios comandos
	 * donde solo importa el id de suscripcion */
	CHECK_VALID_ID(susc_id,subscripsion)
}

int verify_susc_mod(T_dictionary *d, char *message){
	CHECK_VALID_ID(susc_id,subscripsion)
}

int verify_site_id(T_dictionary *d, char *message){
	/* Funcion generica utiliza por varios comandos
	 * donde solo importa el id de sitio y suscripcion */
	CHECK_VALID_ID(susc_id,subscripsion)
	CHECK_VALID_ID(site_id,sitio)
}

int verify_site_list(T_dictionary *d, char *message){
	CHECK_VALID_ID(susc_id,subscripsion)
}

int verify_site_add(T_dictionary *d, char *message){
	CHECK_VALID_ID(susc_id,subscripsion)
}

int verify_site_mod(T_dictionary *d, char *message){
	CHECK_VALID_ID(susc_id,subscripsion)
	CHECK_VALID_ID(site_id,sitio)
}

int verify_site_alldel(T_dictionary *d, char *message){
	CHECK_VALID_ID(susc_id,subscripsion)
}

int verify_server_id(T_dictionary *d, char *message){
	CHECK_VALID_ID(server_id,servidor)
}

int verify_server_mod(T_dictionary *d, char *message){
	CHECK_VALID_ID(server_id,servidor)
}

int verify_server_add(T_dictionary *d, char *message){
	/* Implementar */
	return 1;
}

int verify_ftp_add(T_dictionary *d, char *message){
	CHECK_VALID_ID(site_id,sitio)
	CHECK_VALID_U_P
	return 1;
}

int verify_ftp_list(T_dictionary *d, char *message){
	CHECK_VALID_ID(site_id,sitio)
	return 1;
}
int verify_ftp_del(T_dictionary *d, char *message){
	CHECK_VALID_ID(susc_id,subscripcion)
	CHECK_VALID_ID(ftp_id,ftp)
	return 1;
}
int verify_ftp_mod(T_dictionary *d, char *message){
	CHECK_VALID_ID(ftp_id,ftp)
	CHECK_VALID_U_P
	return 1;
}


int verify_command(char command, T_dictionary *d, char *message){
	/* Verifica que el comando y los parametros recibidos
 	 * sean correctos */
	
	switch(task_c_to_type(command)){
		case T_SUSC_SHOW: return verify_susc_id(d,message);
		case T_SUSC_ADD: return verify_susc_add(d,message);
		case T_SUSC_DEL: return verify_susc_id(d,message);
		case T_SUSC_MOD: return verify_susc_mod(d,message);
		case T_SUSC_STOP: return verify_susc_id(d,message);
		case T_SUSC_START: return verify_susc_id(d,message);

		case T_SITE_LIST: return verify_site_list(d,message);
		case T_SITE_SHOW: return verify_site_id(d,message);
		case T_SITE_ADD: return verify_site_add(d,message);
		case T_SITE_MOD: return verify_site_mod(d,message);
		case T_SITE_DEL: return verify_site_id(d,message);
		case T_SITE_ALLDEL: return verify_site_alldel(d,message);
		case T_SITE_STOP: return verify_site_id(d,message);
		case T_SITE_START: return verify_site_id(d,message);

		case T_SERVER_LIST: return 1;
		case T_SERVER_SHOW: return verify_server_id(d,message);
		case T_SERVER_ADD: return verify_server_add(d,message);
		case T_SERVER_MOD: return verify_server_mod(d,message);
		case T_SERVER_DEL: return verify_server_id(d,message);
		case T_SERVER_STOP: return verify_server_id(d,message);
		case T_SERVER_START: return verify_server_id(d,message);

		case T_FTP_LIST: return verify_ftp_list(d,message);
		case T_FTP_ADD: return verify_ftp_add(d,message);
		case T_FTP_DEL: return verify_ftp_del(d,message);
		case T_FTP_MOD: return verify_ftp_mod(d,message);
		default: return 0;
	}
}

int create_task(T_task **task, char *buffer_rx, char **send_message){
	/* Crea una tarea. Si hay un error lo especifica en
 	   la variable message */
	char value[100];
	char message[200];
	char command;
	int pos=1;
	T_dictionary *data;

	data=(T_dictionary *)malloc(sizeof(T_dictionary));
	dictionary_init(data);
	buffer_to_dictionary(buffer_rx,data,&pos);
	if(!verify_command(buffer_rx[0],data,message)){
		*send_message = (char *)realloc(*send_message,strlen(message) + 1);
		strcpy(*send_message,message);
		return 0;
	} else {
		*task=(T_task *)malloc(sizeof(T_task));
		task_init(*task,task_c_to_type(buffer_rx[0]),data);
		return 1;
	}
}

int recv_all_message(T_server *s, char **rcv_message){
	/* Coordina la recepcion de mensajes grandes del cliente */
	/* El encabezado es de 1 char */
	char buffer[BUFFER_SIZE];
	char printB[BUFFER_SIZE+1];
	int first_message=1;
	uint32_t parce_size;
	int c=0;	// Cantidad de bytes recibidos
	int rcv_message_size=0;

	// Al menos una vez vamos a ingresar
	do{
		if(recv(s->fd_client,buffer,BUFFER_SIZE,0)<=0){
			return 0;
		}
		if(first_message){
			first_message=0;
			_4bytes_to_int(buffer,&rcv_message_size);
			*rcv_message=(char *)realloc(*rcv_message,rcv_message_size);
		}
		_4bytes_to_int(&(buffer[4]),&parce_size);
		memcpy(*rcv_message+c,&(buffer[HEADER_SIZE]),parce_size);
		c += parce_size;
	} while(c < rcv_message_size);
	printf("RECV Mesanje: %s\n",*rcv_message);
	return 1;
}

int send_all_message(T_server *s, char *send_message){
	/* Coordina el envio de los datos aun cuando se necesita
	 * mas de una transmision. send_message debe ser un string que termine
	 * con '\0'. Caso contrario falla */
	char buffer[BUFFER_SIZE];
	char printB[BUFFER_SIZE+1];
	int c=0;	//Cantidad byes enviados
	uint32_t parce_size;
	int send_message_size = strlen(send_message) + 1;

	/*
	printf("Enviaremos al CORE: %i - %s\n",send_message_size, send_message);
	if( send_message[send_message_size-1] != '\0'){
		printf("cloud_send_receive: ERROR. send_message no termina en \\0\n");
		return 0;
	}
	*/

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
	int pos;
	char taskid[TASKID_SIZE];
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

	printf("server_listen\n");
	recv_message = (char *)malloc(10);
	printf("server_listen\n");
	send_message = (char *)malloc(10);
	while(1){
		printf("Esperando conneccion desde el cliente()\n"); //Debemos mantener viva la conexion
		if ((s->fd_client = accept(s->fd_server,(struct sockaddr *)&(s->client),&(s->sin_size)))<0) {
			printf("error en accept()\n");
			exit(1);
		}

		// Aguardamos continuamente que el cliente envie un comando
		while(recv_all_message(s,&recv_message)){
			printf("Recibimos del CORE-%s-\n",recv_message);
			if(recv_message[0] == 't'){
				/* nos solicitan el estado de un task */
				pos=1;
				parce_data(recv_message,'|',&pos,taskid);
				printf("Recuperamos TASKID -%s- del mensaje -%s-\n",taskid,recv_message);
				server_get_task(s,(T_taskid *)taskid,&send_message);
			} else if(recv_message[0] == 'c'){
				/* nos solicitan un chequeo */
				server_check(s,&send_message);
			} else {
				/* Creamos el task si los datos son correctos */
				if (create_task(&task,recv_message,&send_message)){
					server_add_task(s,task,&send_message);
				}
			}
			send_all_message(s,send_message);
		}
		close(s->fd_client);
	}
}

void server_init(T_server *s, T_list_site *sites, T_list_worker *workers,
	T_list_proxy *proxys, T_db *db, T_config *config, T_logs *logs){

	s->sites = sites;
	s->proxys = proxys;
	s->workers = workers;
	s->db = db;
	s->config = config;
	s->logs = logs;
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
