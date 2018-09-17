#include "server.h"

void server_add_task(T_server *s, T_task *t, char **message, unsigned int *message_size){

	pthread_mutex_lock(&(s->mutex_heap_task));
		printf("ADD_TASK - %s\n",task_get_id(t));
		heap_task_push(&(s->tasks_todo),t);
		*message=(char *)realloc(*message,(TASKID_SIZE + 2)*sizeof(char));
		*message_size = TASKID_SIZE + 2;
		sprintf(*message,"1%s",task_get_id(t));
	pthread_mutex_unlock(&(s->mutex_heap_task));
}

void *server_do_task(void *param){
	T_task *task;
	T_server *s= (T_server *)param;

	printf("EJECUTAMOS un TASK!!!!\n");
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
			*size=2;
			*result=(char *)realloc(*result,*size * sizeof(char));
			/* Verificamos si esta en la cola de tareas pendientes */
			if(heap_task_exist(&(s->tasks_todo),(char *)taskid)){
				/* Tarea existe y esta en espera */
				strcpy(*result,"2");
			} else {
				/* Tarea no existe */
				strcpy(*result,"0");
			}
		} else {
			/* Tarea existe y ha finalizado */
			*size = strlen(task_get_result(task));
			*result = (char *)realloc(*result,(*size + 2) * sizeof(char));
			sprintf(*result,"1%s",task_get_result(task));
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

	printf("buffer_to_dictionary-%i-%s-\n",*pos,message);
	largo = strlen(message);
	while(*pos<largo){
		parce_data(message,'|',pos,name);
		parce_data(message,'|',pos,value);
		dictionary_add(data,name,value);
	}
}

int create_task(T_task **task, char *buffer_rx, char **message, unsigned int *message_size){
	/* Crea una tarea. Si hay un error lo especifica en
 	   la variable message */
	char value[100];
	char command;
	int pos;
	T_dictionary *data;

	*task=(T_task *)malloc(sizeof(T_task));
	data=(T_dictionary *)malloc(sizeof(T_dictionary));
	dictionary_init(data);
	pos = 1;
	buffer_to_dictionary(buffer_rx,data,&pos);
	task_init(*task,task_c_to_type(buffer_rx[0]),data);
	return 1;
}

int recv_all_message(T_server *s, char **recv_message, int *recv_message_size){
	/* Coordina la recepcion de mensajes grandes del cliente */
	/* El encabezado es de 1 char */
	char buffer[BUFFER_SIZE];
	int pos;
	int fin='1';

	*recv_message_size = 0;
	while(fin == '1'){
		printf("Esperamos comando nuevo\n");
		if(recv(s->fd_client,buffer,BUFFER_SIZE,0)<0){
			return 0;
		}
		printf("El buffer es:%s\n",buffer);
		pos = *recv_message_size;
		*recv_message_size += BUFFER_SIZE - HEAD_SIZE;
		*recv_message=(char *)realloc(*recv_message, *recv_message_size * sizeof(char));
		memcpy(recv_message[pos],&(buffer[HEAD_SIZE]),BUFFERSIZE - HEAD_SIZE);
		fin = buffer[0];
		/* Enviamos al cliente confirmacion de resepcion de mas datos
 		 * solo si en el header venia un 1 */
		if(fin == '1')
			send(s->fd_client,"1\0", BUFFER_SIZE,0);
	}
	printf("recv_all_message:-%s-\n",*recv_message);
	return 1;
}

int send_all_message(T_server *s, char *message, int message_size){
	/* Coordina el envio de los datos aun cuando se necesita
	 * mas de una transmision */
	char buffer_tx[BUFFER_SIZE];
	char buffer_rx[BUFFER_SIZE];
	char *p;
	int c;

	p = message;
	c = 0;
	printf("Enviamos la respuesta al Core\n");
	while(c < message_size){
		if((message_size - c + HEAD_SIZE) < BUFFER_SIZE){
			memcpy(buffer_tx + HEAD_SIZE,p,message_size - c);
			buffer_tx[0] = '0';
			c += message_size - c;
		} else{
			memcpy(buffer_tx + HEAD_SIZE,p,BUFFER_SIZE - HEAD_SIZE);
			buffer_tx[0] = '1';
			c += BUFFER_SIZE - HEAD_SIZE;
			p += c;
		}
		printf("send_all_message-enviamos %s\n",buffer_tx);
		if(!send(s->fd_client,buffer_tx,BUFFER_SIZE,0))
			return 0;
		/* Esperamos recibir confirmacion para enviar mas datos
 		 * solo si hemos enviado un 1 en el header */
		if(buffer_tx[0] == '1')
			if(!recv(s->fd_client,buffer_rx,BUFFER_SIZE,0)>0)
				return 0;
		printf("send_all_message-recibimos %s\n",buffer_rx);
		if(buffer_rx[0] == '0')
			return 0;
	}
	return 1;
}

void *server_listen(void *param){
	char *recv_message = NULL;
	char *send_message = NULL;
	int recv_message_size = 0;
	int send_message_size = 0;
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
			} else {
				/* Creamos el task  */
				if(create_task(&task,recv_message,&send_message,&send_message_size)){
					server_add_task(s,task,&send_message,&send_message_size);
				}
			}
			printf("Informamos al CORE\n");
			send_all_message(s,send_message,send_message_size);

			/* Quizas liberar la memoria nos ea necesario 
 
			printf("liberamos memoria\n");
  			free(recv_message);
			free(send_message);
			recv_message=NULL;
			send_message=NULL;
			recv_message_size=0;
			send_message_size=0;
			*/
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
