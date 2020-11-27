#include "server.h"

/***********************************
 * 		Chequeos varios			   *
 ***********************************/

int check_login(T_dictionary *d, char *message){
	/* De momento retorna siempre 1 */
	return 1;
}

int check_site_show(T_dictionary *d, char *message){
	CHECK_VALID_ID(namespace_id,subscripcion)
	CHECK_VALID_ID(namespace_id,sitio)
	return 1;
}

int check_site_list(T_dictionary *d, char *message){
	CHECK_VALID_ID(namespace_id,namespace)
	return 1;
}

int check_namespace_show(T_dictionary *d, char *message){
	CHECK_VALID_ID(namespace_id,namespace)
	return 1;
}

int check_namespace_add(T_dictionary *d, char *message){
	CHECK_VALID_NAME(name,namespace)
	return 1;
}

int check_namespace_mod(T_dictionary *d, char *message){
	CHECK_VALID_ID(namespace_id,namespace)
	return 1;
}

int check_namespace_del(T_dictionary *d, char *message){
	CHECK_VALID_ID(namespace_id,namespace)
	return 1;
}

int check_site_mod(T_dictionary *d, char *message){
	CHECK_VALID_ID(namespace_id,namespace)
	CHECK_VALID_ID(site_id,sitio)
	return 1;
}

int check_site_add(T_dictionary *d, char *message){
	CHECK_VALID_ID(namespace_id,namespace)
	CHECK_VALID_SITE_NAME(name)
	return 1;
}

int check_site_del(T_dictionary *d, char *message){
	CHECK_VALID_ID(namespace_id,namespace)
	CHECK_VALID_ID(site_id,sitio)
	return 1;
}

int check_cloud_show(T_dictionary *d, char *message){
	CHECK_VALID_ID(cloud_id,nube)
	return 1;
}

int check_ftp_list(T_dictionary *d, char *message){
	CHECK_VALID_ID(namespace_id,namespace)
	CHECK_VALID_ID(site_id,sitio)
	return 1;
}

int check_ftp_del(T_dictionary *d, char *message){
	CHECK_VALID_ID(namespace_id,namespace)
	CHECK_VALID_ID(site_id,sitio)
	CHECK_VALID_ID(ftp_id,usuario ftp)
	return 1;
}

int check_ftp_add(T_dictionary *d, char *message){
	CHECK_VALID_ID(namespace_id,namespace)
	CHECK_VALID_ID(site_id,sitio)
	if(!valid_passwd(dictionary_get(d,"passwd"))){
		strcpy(message,"{\"task\":\"\",\"status\":\"ERROR\",\"data\":\"Password invalida\"}");
		return 0;
	}
	if(!valid_ftp_name(dictionary_get(d,"name"))){
		strcpy(message,"{\"task\":\"\",\"status\":\"ERROR\",\"data\":\"nombre usuario invalido\"}");
		return 0;
	}
	return 1;
}

int check_ftp_mod(T_dictionary *d, char *message){
	CHECK_VALID_ID(namespace_id,namespace)
	CHECK_VALID_ID(site_id,sitio)
	CHECK_VALID_ID(ftp_id,usuario ftp)
	return 1;
}

void server_add_task(T_server *s, T_task *t){

	printf("Agregamos el task a la cola\n");
	pthread_mutex_lock(&(s->mutex_heap_task));
		heap_task_push(&(s->tasks_todo),t);
	pthread_mutex_unlock(&(s->mutex_heap_task));
}

uint32_t server_num_tasks(T_server *s){
		printf("cantidad tareas: %u,%u\n",heap_task_size(&(s->tasks_todo)), bag_task_size(&(s->tasks_done)));
		return (heap_task_size(&(s->tasks_todo)) + bag_task_size(&(s->tasks_done)));
}

void *server_purge_done(void *param){
	/* Se encarga de purgar cada 10 segundos la estructura
	   de tareas finalizadas */

	T_server *s= (T_server *)param;
	while(1){
		sleep(1);
		pthread_mutex_lock(&(s->mutex_bag_task));
			bag_task_timedout(&(s->tasks_done),
			config_task_timeout(s->config));
		pthread_mutex_unlock(&(s->mutex_bag_task));
	}
}

void *server_do_task(void *param){
	T_server *s= (T_server *)param;

	while(1){
		//sleep(20);
		//printf("Corremos el task\n");
		pthread_mutex_lock(&(s->mutex_heap_task));
			s->runningTask = heap_task_pop(&(s->tasks_todo));
		pthread_mutex_unlock(&(s->mutex_heap_task));
		if(s->runningTask != NULL){
			/* Si la tarea hace mas de un minuto que esta en cola
			   vence por timeout */
			if(60 < difftime(time(NULL),task_get_time(s->runningTask)))
				task_done(s->runningTask,HTTP_502,M_TIMEOUT_ERROR);
			else {
				pthread_mutex_lock(&(s->mutex_lists));
					task_run(s->runningTask,s->sites,s->workers,s->proxys,s->db,s->config,s->logs);
				pthread_mutex_unlock(&(s->mutex_lists));
			}
			pthread_mutex_lock(&(s->mutex_bag_task));
				bag_task_add(&(s->tasks_done),s->runningTask);
				s->runningTask = NULL;
				bag_task_print(&(s->tasks_done));
			pthread_mutex_unlock(&(s->mutex_bag_task));
		}
	}
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

int verify_namespace_show(T_dictionary *d, char *message){
	CHECK_VALID_ID(namespace_id,subscripsion)
}

int verify_namespace_add(T_dictionary *d, char *message){
	CHECK_VALID_ID(namespace_id,subscripsion)
	if(!valid_size(dictionary_get(d,"web_quota"))){
		sprintf(message,"300|\"code\":\"300\",\"info\":\"tamano en bytes invalido\"");
		return 0;
	}
	if(!valid_size(dictionary_get(d,"web_sites"))){
		sprintf(message,"300|\"code\":\"300\",\"info\":\"cantidad sitios invalido\"");
		return 0;
	}
	return 1;
}

void server_url_error(char *result, int *ok){
	strcpy(result,"{\"task\":\"\",\"status\":\"ERROR\",\"data\":\"api call invalid\"}");
	*ok=0;
}

static int send_page(struct MHD_Connection *connection, const char *page){
	int ret;
	struct MHD_Response *response;

	response = MHD_create_response_from_buffer(strlen(page), (void *)page,
			MHD_RESPMEM_PERSISTENT);
	if (!response)
		return MHD_NO;

	MHD_add_response_header(response,"Access-Control-Allow-Origin","*");
	MHD_add_response_header(response,"Access-Control-Allow-Methods","POST, PUT, GET, DELETE");
	MHD_add_response_header(response,"Access-Control-Allow-Headers","Access-Control-Allow-Origin");

	printf("RESPONDEMOS AL CLIENTE\n");
	ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
	MHD_destroy_response (response);

	return ret;
}

void server_get_task(T_server *r, T_taskid *taskid, char **message, unsigned int *message_size){
	/* Retorna en formato json el resultado de la finalizacion de un task */
	T_task *task = NULL;
	char status[30];
	unsigned int total_size;

	printf("Entramos a buscar TASK\n");
	pthread_mutex_lock(&(r->mutex_bag_task));
	pthread_mutex_lock(&(r->mutex_heap_task));

	/* Buscamos en la bolsa de tareas finalizadas. */
	printf("Buscando en bag done\n");
	bag_task_print(&(r->tasks_done));
	task = bag_task_pop(&(r->tasks_done),taskid);
	if(task == NULL){
		/* Buscamos entonces en la cola de pendientes. En este caso
		   se retorna una copia del task pero no se quita de la cola */
		printf("Buscando en cola todo\n");
		task = heap_task_exist(&(r->tasks_todo),(char *)taskid);
	} else if(task == NULL){
		/* Verificamos si esta actualmente corriendo */
		printf("Verificamos task corriendo\n");
		task = r->runningTask;
		printf("Verificamos task corriendo paso\n");
	}

	/* Ya obtenido o no el task, informamos */
	if(task == NULL){
		printf("Task es null\n");
		sprintf(*message,"{\"taskid\":\"%s\",\"status\":\"inexist\"}",taskid);
	} else {
		/* Armamos en message el resultado de la terea */
		task_json_result(task,message);
		printf("REST_SERVER_GET_TASK: %s\n",*message);
		/*
		if(task_get_status(task) > T_WAITING)
			task_destroy(&task);
		*/
	}

	pthread_mutex_unlock(&(r->mutex_heap_task));
	pthread_mutex_unlock(&(r->mutex_bag_task));
}

static int handle_POST(struct MHD_Connection *connection,
			const char *url,
			struct connection_info_struct *con_info){
	int pos=1;
	int ok=1;
	char value[100];
	char *result = (char *)malloc(TASKRESULT_SIZE);
	unsigned int size_result = TASKRESULT_SIZE;
	T_task *task=NULL;
	T_taskid *taskid;

	printf("Handle_post ENTRO\n");

	/* Le pasamos el puntero del diccionario del
	   parametro con_info a la variable task. OJO
	   que entonces ya no es responsabilidad del metodo
	   eliminar la estructura de diccionario. Sino que pasa
	   a ser responsabilidad del task. */

	task = (T_task *)malloc(sizeof(T_task));

	parce_data((char *)url,'/',&pos,value);
	task = (T_task *)malloc(sizeof(T_task));
	task_init(task);
	/* PARA LOGIN */
	if(0 == strcmp("login",value)){
		parce_data((char *)url,'/',&pos,value);
		if(strlen(value)>0){
			ok=0;
			server_url_error(result,&ok);
		} else {
			if(ok = check_login(con_info->data,result)){
				task_set(task,T_LOGIN,con_info->data);
			}
		}
	}
	/* PARA LOS NAMESPACE */
	if(0 == strcmp("namespace",value)){
		printf("Handle_POST namespacees\n");
		parce_data((char *)url,'/',&pos,value);
		if(strlen(value)>0){
			dictionary_add(con_info->data,"namespace_id",value);
			parce_data((char *)url,'/',&pos,value);
			if(strlen(value)>0){
				if(0 == strcmp("site",value)){
					printf("Handle_POST sitios\n");
					parce_data((char *)url,'/',&pos,value);
					if(strlen(value)>0){
						dictionary_add(con_info->data,"site_id",value);
						parce_data((char *)url,'/',&pos,value);
						if(strlen(value)>0){
							if(0 == strcmp("ftp_user",value)){
								parce_data((char *)url,'/',&pos,value);
								if(strlen(value)>0){
									/* EDICION FTP */
									dictionary_add(con_info->data,"ftp_id",value);
									if(ok = check_ftp_mod(con_info->data,result))
										task_set(task,T_FTP_MOD,con_info->data);
								} else {
									/* ALTA FTP */
									if(ok = check_ftp_add(con_info->data,result))
										task_set(task,T_FTP_ADD,con_info->data);
								}
							} else {
								server_url_error(result,&ok);
							}
						} else {
							/* EDICION SITIO */
							if(ok = check_site_mod(con_info->data,result))
								task_set(task,T_SITE_MOD,con_info->data);
						}
					} else {
						/* ALTA SITIO */
						if(ok = check_site_add(con_info->data,result))
							task_set(task,T_SITE_ADD,con_info->data);
					}
				} else {
					server_url_error(result,&ok);
				}
			} else {
				/* EDICION NAMESPACE */
				dictionary_add(con_info->data,"namespace_id",value);
				if(ok = check_namespace_mod(con_info->data,result))
					task_set(task,T_NAMESPACE_MOD,con_info->data);
			}
		} else {
			/* ALTA NAMESPACE */
			printf("Handle_POST alta namespace\n");
			if(ok = check_namespace_add(con_info->data,result))
				task_set(task,T_NAMESPACE_ADD,con_info->data);
		}
	} else {
		strcpy(result,"URL mal ingresada");
		server_url_error(result,&ok);
	}

	if(ok){
		if(server_num_tasks(&server) < 200 ){
			server_add_task(&server,task);
			sprintf(result,"{\"task\":\"%s\",\"status\":\"TODO\"}",task_get_id(task));
		} else {
			sprintf(result,"{\"task\":\"%s\",\"status\":\"ERROR\",\"data\":\"Superando limite de tareas\"}");
		}
	} else
		task_destroy(&task);

	printf("Enviando al cliente REST: %s\n",result);
	send_page (connection,result);
	return ok;
}

static int handle_DELETE(struct MHD_Connection *connection, const char *url){

	char value[100];
	int pos=1;
	int ok=1;
	T_dictionary *data;
	char *result = (char *)malloc(TASKRESULT_SIZE);
	unsigned int size_result = TASKRESULT_SIZE;
	T_task *task=NULL;
	T_taskid *taskid;

	data = malloc(sizeof(T_dictionary));
	dictionary_init(data);
	task = (T_task *)malloc(sizeof(T_task));
	task_init(task);
	printf("Handle_POST\n");

	parce_data((char *)url,'/',&pos,value);
	if(strlen(value)>0){
		if(0 == strcmp("namespace",value)) {
			parce_data((char *)url,'/',&pos,value);
			if(strlen(value)>0){
				dictionary_add(data,"namespace_id",value);
				parce_data((char *)url,'/',&pos,value);
				if(strlen(value)>0){
					if(0 == strcmp("site",value)) {
						parce_data((char *)url,'/',&pos,value);
						if(strlen(value)>0) {
							dictionary_add(data,"site_id",value);
							parce_data((char *)url,'/',&pos,value);
							if(strlen(value)>0) {
								if(0 == strcmp("ftp_user",value)) {
									parce_data((char *)url,'/',&pos,value);
									if(strlen(value)>0) {
										dictionary_add(data,"ftp_id",value);
										/* BORRADO FTP */
										if(ok = check_ftp_del(data,result))
											task_set(task,T_FTP_DEL,data);
											//task_set_type(task,T_FTP_DEL);
									} else {
										server_url_error(result,&ok);
									}
								} else {
									server_url_error(result,&ok);
								}
							} else {
								/* BORRADO SITIO */
								if(ok = check_site_del(data,result))
									task_set(task,T_SITE_DEL,data);
									//task_set_type(task,T_SITE_DEL);
							}
						} else {
							server_url_error(result,&ok);
						}
					} else {
						server_url_error(result,&ok);
					}
				} else {
					/* BORRADO NAMESPACE */
					if(ok = check_namespace_del(data,result))
						task_set(task,T_NAMESPACE_DEL,data);
						//task_set_type(task,T_NAMESPACE_DEL);
				}
			} else {
				server_url_error(result,&ok);
			}
		} else {
			server_url_error(result,&ok);
		}
	}

	if(ok){
		if(server_num_tasks(&server) < 200 ){
			server_add_task(&server,task);
			sprintf(result,"{\"task\":\"%s\",\"status\":\"TODO\"}",task_get_id(task));
		} else {
			sprintf(result,"{\"task\":\"%s\",\"status\":\"ERROR\",\"data\":\"Superando limite de tareas\"}");
		}
	} else {
		task_destroy(&task);
	}
	printf("Enviando al cliente REST: %s\n",result);
	send_page (connection,result);
	return ok;
}

static int handle_GET(struct MHD_Connection *connection, const char *url){
	char value[100];
	int pos=1;
	T_dictionary *data;
	char *result = (char *)malloc(TASKRESULT_SIZE);
	unsigned int size_result = TASKRESULT_SIZE;
	T_task *task = NULL;
	T_taskid *taskid;
	int ok=1;   // Resultado a retornar la funcion
	int isTaskStatus =0;	// Cualquier GET excepto el que solisita el estadod e un task se encola.

	printf("Handle_get ENTRO: %s\n",url);

	data = malloc(sizeof(T_dictionary));
	dictionary_init(data);
	task = (T_task *)malloc(sizeof(T_task));
	task_init(task);
	printf("direccion del task %p\n",task);

	parce_data((char *)url,'/',&pos,value);

	/* el manejo de servidores */
	if(0 == strcmp("servers",value)){
		printf("Handle_GET: servidores\n");
		parce_data((char *)url,'/',&pos,value);
		if(strlen(value)>0){
			dictionary_add(data,"server_id",value);
			parce_data((char *)url,'/',&pos,value);
			if(0 == strcmp("stop",value)){
				task_set(task,T_SERVER_STOP,data);
			} else if(0 == strcmp("start",value)){
				task_set(task,T_SERVER_START,data);
			} else
				task_set(task,T_SERVER_SHOW,data);
		} else
			task_set(task,T_SERVER_LIST,data);
	/* Para el listado de sitios todos */
	} else if(0 == strcmp("sites",value)) {
		parce_data((char *)url,'/',&pos,value);
		if(strlen(value)>0){
			server_url_error(result,&ok);
		} else {
			printf("PASO LIST SITES\n");
			task_set(task,T_SITE_LIST,data);
			//task_set_type(task,T_SITE_LIST);
		}
	/* PARA LOS NAMESPACES */
	} else if(0 == strcmp("namespace",value)){
		printf("Handle_GET: namespaces\n");
		parce_data((char *)url,'/',&pos,value);
		if(strlen(value)>0){
			dictionary_add(data,"namespace_id",value);
			parce_data((char *)url,'/',&pos,value);
			if(strlen(value)>0){
				printf("sitios\n");
				if(0 == strcmp("site",value)){
					parce_data((char *)url,'/',&pos,value);
					if(strlen(value)>0){
						dictionary_add(data,"site_id",value);
						parce_data((char *)url,'/',&pos,value);
						if(strlen(value)>0){
							if(0 == strcmp("ftp_user",value)){
								parce_data((char *)url,'/',&pos,value);
								if(strlen(value)>0){
									server_url_error(result,&ok);
								} else
									/* Listado ftp */
									if(ok = check_ftp_list(data,result))
										task_set(task,T_FTP_LIST,data);
							} else if (0 == strcmp("stop",value)){
								/* Detenemos sitio */
								task_set(task,T_SITE_STOP,data);
								ok = 1;
							} else if (0 == strcmp("start",value)){
								/* Iniciamos sitio */
								task_set(task,T_SITE_START,data);
								ok = 1;
							} else
								server_url_error(result,&ok);
						} else
							/* Show sitio */
							if(ok = check_site_show(data,result))
								task_set(task,T_SITE_SHOW,data);
					} else {
						/* Listado de sitios */
						printf("lllll de sitios\n");
						if(ok = check_site_list(data,result)){
							printf("Listado de sitios\n");
							task_set(task,T_SITE_LIST,data);
						}
					}
				} else {
					server_url_error(result,&ok);
				}
			} else {
				/* Informacion de un namespace */
				printf("Informe de un namespace\n");
				if(ok = (check_namespace_show(data,result)))
					task_set(task,T_NAMESPACE_SHOW,data);
			}
		} else {
			/* Listado de namespaces*/
			printf("Listado de namespace\n");
			task_set(task,T_NAMESPACE_LIST,data);
		}
	/* PARA LOS TASK */
	} else if(0 == strcmp("task",value)) {
		printf("Paso 1 Santiago\n",url);
		/* Se solicita info de un task */
		isTaskStatus =1;
		task_destroy(&task);
		printf("Task destruido\n");
		parce_data((char *)url,'/',&pos,value);
		if(strlen(value)>0){
			printf("handle_GET server_get_task\n");
			server_get_task(&server,(T_taskid *)value,&result,&size_result);
		} else
			strcpy(result,"{task incorrecto}");

	/* PARA CUALQUIER OTRA COSA */
	} else {
		printf("Handle_GET: cualquier Cosa\n");
		server_url_error(result,&ok);
		printf("Handle_GET: cualquier Cosa paso\n");
	}

	if(ok){
		if(!isTaskStatus){
			if(server_num_tasks(&server) < 200 ){
				sprintf(result,"{\"task\":\"%s\",\"status\":\"TODO\"}",task_get_id(task));
				server_add_task(&server,task);
			} else {
				sprintf(result,"{\"task\":\"%s\",\"status\":\"ERROR\",\"data\":\"Superando limite de tareas\"}");
			}
		}
	} else {
		printf("Destruimos task: %p\n",task);
		task_destroy(&task);
		printf("Destruimos task fin\n");
	}

	printf("Enviando al cliente REST: %s\n",result);
	send_page (connection, result);
	return ok;
}

static int iterate_post (void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
			const char *filename, const char *content_type,
			const char *transfer_encoding, const char *data, uint64_t off,
			size_t size){

	struct connection_info_struct *con_info = coninfo_cls;

	if(strlen(data)>0){
		printf("iterate_post: Agregamos al diccionario %s->%s\n",(char *)key,(char *)data);
		dictionary_add(con_info->data,(char *)key,(char *)data);
		return MHD_YES;
	} else {
		return MHD_NO;
	}
}

static void request_completed (void *cls, struct MHD_Connection *connection,
			   void **con_cls, enum MHD_RequestTerminationCode toe){

	struct connection_info_struct *con_info = *con_cls;
	if (NULL == con_info)
		return;
	if (con_info->connectiontype == POST){
		MHD_destroy_post_processor (con_info->postprocessor);
	}
	if (con_info->connectiontype != POST){
		dictionary_destroy(&(con_info->data));
	}
	free (con_info);
	*con_cls = NULL;
}

static int answer_to_connection (void *cls, struct MHD_Connection *connection,
				const char *url, const char *method,
				const char *version, const char *upload_data,
				size_t *upload_data_size, void **con_cls){

	if (NULL == *con_cls){
		struct connection_info_struct *con_info;
		con_info = malloc (sizeof (struct connection_info_struct));
		con_info->data = malloc (sizeof (T_dictionary));
		dictionary_init(con_info->data);
		if (NULL == con_info){
			printf("por aca ERROR 1\n");
			return MHD_NO;
		}
		if (0 == strcmp (method, "POST")){
			printf("por aca\n");
			con_info->postprocessor =
				MHD_create_post_processor (connection, POSTBUFFERSIZE,
				iterate_post, (void *) con_info);

			if (NULL == con_info->postprocessor){
				printf("por aca ERROR: NO SOPORTA JSON\n");
				free (con_info);
				return MHD_NO;
			}
			con_info->connectiontype = POST;
		} else {
			con_info->connectiontype = GET;
		}
		*con_cls = (void *) con_info;
		return MHD_YES;
	}
	printf("API_REST url: %s\n",url);

	if (0 == strcmp (method, "GET")){
		handle_GET(connection,url);
		return MHD_YES;
	}

	if (0 == strcmp (method, "DELETE")){
		handle_DELETE(connection,url);
		return MHD_YES;
	}

	if (0 == strcmp (method, "POST")){
		struct connection_info_struct *con_info = *con_cls;
		if (*upload_data_size != 0){
			MHD_post_process (con_info->postprocessor, upload_data,
					*upload_data_size);
			*upload_data_size = 0;
			return MHD_YES;
		} else {
			printf("Entramos en Handle POST\n");
			handle_POST(connection,url,con_info);
			return MHD_YES;
		}
	}
	return send_page(connection, "TODO MAL");
}

void *rest_server_start(void *param){

	T_server *r= (T_server *)param;

	r-> rest_daemon = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_DEBUG,
			REST_PORT, NULL, NULL, &answer_to_connection, NULL, MHD_OPTION_NOTIFY_COMPLETED,
			request_completed, NULL, MHD_OPTION_END);
}

void server_init(T_server *s, T_lista *sites, T_lista *workers,
	T_lista *proxys, T_db *db, T_config *config, T_logs *logs){

	s->sites = sites;
	s->proxys = proxys;
	s->workers = workers;
	s->runningTask = NULL;
	s->db = db;
	s->config = config;
	s->logs = logs;
	heap_task_init(&(s->tasks_todo));
	bag_task_init(&(s->tasks_done));
	if(0 != pthread_create(&(s->thread), NULL, &rest_server_start, s)){
		exit(2);
	}
	if(0 != pthread_create(&(s->purge_done), NULL, &server_purge_done, s)){
		printf ("Imposible levantar el hilo purge_done\n");
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
