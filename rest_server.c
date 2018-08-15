#include "rest_server.h"

/* La variable server_rest debe ser global */

void rest_server_add_job(T_rest_server *r, T_job *j){
	printf("Agregamos el JOB: %s a la lista\n",job_get_id(j));
	list_job_add(&(r->jobs),j);

	/* Para debug imprimimis la lista hasta el momento */
	list_job_print(&(r->jobs));
}

static int send_page(struct MHD_Connection *connection, const char *page){
	int ret;
	struct MHD_Response *response;

	response = MHD_create_response_from_buffer(strlen(page), (void *)page,
			MHD_RESPMEM_PERSISTENT);
	if (!response)
		return MHD_NO;

	ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
	MHD_destroy_response (response);

	return ret;
}

static int handle_GET(struct MHD_Connection *connection, const char *url){
	char value[100];
	int pos=1;
	int data_size=1000;
	char *data;
	T_job *job;
	T_jobid *jobid;

	/* El token de momento lo inventamos
 	   pero deberia venir en el header del mensaje */
	T_jobtoken token;
	random_token(token);

	printf("Se trata de un GET\n");
	parce_data((char *)url,'/',&pos,value);
	printf("valor obtenido: %s\n",value);

	data = (char *)malloc(data_size);

	if(0 == strcmp("sites",value)){
		parce_data((char *)url,'/',&pos,value);
		if(strlen(value)>0){
			printf("Nos piden informacion sobre el sitio: %s\n",value);
			//return send_page (connection, "Acciones sobre el sitio");
			 job = (T_job *)malloc(sizeof(T_job));
			job_init(job,&token,J_GET_SITE,value);
		} else {
			printf("Nos piden listar los sitios: %s\n",value);
			job = (T_job *)malloc(sizeof(T_job));
			job_init(job,&token,J_GET_SITES,value);
		}
		/* Solicitamos el listado de sitios */
	} else {
		/* ERROR de protocolo. URL mal confeccionada */
		printf("Error en la URL\n");
		return send_page (connection, "ERROR");
		return 0;
	}
	rest_server_add_job(&rest_server,job);
	send_page (connection, "1");
	return 1;
}

static int iterate_post (void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
			const char *filename, const char *content_type,
			const char *transfer_encoding, const char *data, uint64_t off,
			size_t size){

	struct connection_info_struct *con_info = coninfo_cls;

	if (0 == strcmp (key, "name")){
		if ((size > 0) && (size <= MAXNAMESIZE)){
			char *answerstring;
			answerstring = malloc (MAXANSWERSIZE);
			if (!answerstring)
				return MHD_NO;
			snprintf (answerstring, MAXANSWERSIZE, "TODO OK", data);
			con_info->answerstring = answerstring;
		} else
			con_info->answerstring = NULL;
		return MHD_NO;
	}
	return MHD_YES;
	
}

static void request_completed (void *cls, struct MHD_Connection *connection,
			   void **con_cls, enum MHD_RequestTerminationCode toe){

	struct connection_info_struct *con_info = *con_cls;
	if (NULL == con_info)
		return;
	if (con_info->connectiontype == POST){
		MHD_destroy_post_processor (con_info->postprocessor);
		if (con_info->answerstring)
			free (con_info->answerstring);
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
		if (NULL == con_info)
			return MHD_NO;
		con_info->answerstring = NULL;

		if (0 == strcmp (method, "POST")){
			con_info->postprocessor = 
				MHD_create_post_processor (connection, POSTBUFFERSIZE,
				iterate_post, (void *) con_info);

			if (NULL == con_info->postprocessor){
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

	if (0 == strcmp (method, "GET")){
		printf("Entrando manejador del GET %s\n",url);
		return handle_GET(connection,url);
	}
	if (0 == strcmp (method, "POST")){
		struct connection_info_struct *con_info = *con_cls;
		if (*upload_data_size != 0){
			MHD_post_process (con_info->postprocessor, upload_data,
					*upload_data_size);
			*upload_data_size = 0;
			return MHD_YES;
		} else if (NULL != con_info->answerstring)
			return send_page (connection, con_info->answerstring);
	}
	return send_page(connection, "TODO MAL");
}

void *rest_server_start(void *param){
	
	T_rest_server *r= (T_rest_server *)param;

	printf("Levantando el demonio REST\n");
	r-> rest_daemon = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_DEBUG,
			80, NULL, NULL, &answer_to_connection, NULL, MHD_OPTION_END);
}

void rest_server_init(T_rest_server *r){
	list_job_init(&(r->jobs));
	if(0 != pthread_create(&(r->thread), NULL, &rest_server_start, r)){
		printf ("Imposible levantar el servidor REST\n");
		exit(2);
	}
}


