#include "json.h"

void json_mysql_result_row(MYSQL_ROW *row, char *col_names[50], int cant_cols, char **message){
	/* Retorna en formato json una fila */
	int i = 0;
	int e = 0;
	char element[1000];

	printf("Estamos aca 2:%i\n",cant_cols);
	dim_copy(message,"{");
	i = 0;
	while(i<cant_cols){
		e = 1;
		printf("COLUMNAS %s\n",col_names[i]);
		sprintf(element,"\"%s\":\"%s\",",col_names[i],row[i]);
		dim_concat(message,element);
		i++;
	}
	/* Eliminamos la ultima "," entre variables */
	if(e){
        (*message)[strlen(*message) - 1] = '}';
	} else {
		dim_concat(message,"}");
	}
}


void json_mysql_result(MYSQL_RES *result, char **message){
	/* Retorna un resultado Mysql como JSON */
	MYSQL_ROW *row;
	MYSQL_FIELD *field;
	int e1,e2 = 0;
	char *element = NULL;
	int fields = 0;
	int i;
	char *col_names[50];

	printf("Estamos aca\n");
	/* Cargamos el array con los nombres de los campos */
	i = 0;
	while(field = mysql_fetch_field(result)){
		col_names[i] = field->name;
		fields++;
		i++;
	}
	printf("Estamos aca 2\n");

	dim_init(message);
	dim_concat(message,"[");
	e2 = 0;
	printf("Estamos aca 3\n");
	while(row = mysql_fetch_row(result)){
		json_mysql_result_row(row, col_names, fields, &element);
		e2 = 1;
		dim_concat(message,element);
		dim_concat(message,",");
	}
	/* Eliminamos la ultima "," entre elementos */
	if(e2){
        (*message)[strlen(*message) - 1] = ']';
    } else {
		dim_concat(message,"]");
    }
}

void json_servers(char **data, T_lista *workers, T_lista *proxys, T_db *db){
	/* Retorna los id de todos los workers y proxys existentes y alguna otra info relevante */
	/* Realoca la memoria de data y retorna el tamano en size, si es que no entran los datos */

	T_worker *worker;
	T_proxy *proxy;
	char status[20];
	int real_size = 100;

	*data = (char *) realloc(*data,real_size * sizeof(char));
	strcpy(*data,"200|[");
	lista_first(workers);
	while(!lista_eol(workers)){
		worker = lista_get(workers);
		if((100 + strlen(*data)) > real_size){
			/* Los datos pueden superan el espacio de data */
			real_size += 100;
			*data = (char *) realloc(*data,real_size * sizeof(char));
		}
		itowstatus(worker_get_status(worker),status);
		sprintf(*data,"%s{\"id\":%lu,\"name\":\"%s\",\"rol\":\"worker\",\"status\":\"%s\"},",
			*data,worker_get_id(worker),worker_get_name(worker),status);
		lista_next(workers);
	}
	lista_first(proxys);
	while(!lista_eol(proxys)){
		proxy = lista_get(proxys);
		if((100 + strlen(*data)) > real_size){
			/* Los datos pueden superan el espacio de data */
			real_size += 100;
			*data = (char *) realloc(*data,real_size * sizeof(char));
		}
		itopstatus(proxy_get_status(proxy),status);
		sprintf(*data,"%s{\"id\":%lu,\"name\":\"%s\",\"rol\":\"proxy\",\"status\":\"%s\"},",
			*data,proxy_get_id(proxy),proxy_get_name(proxy),status);
		lista_next(proxys);
	}
	(*data)[strlen(*data)-1] = ']';
}


int json_worker(char **data, T_worker *worker, T_db *db){
	/* Retorna todos los datos de un worker
 	 * Realoca la memoria de data y retorna el tamano en
	 * size, si es que no entran los datos */

	T_site *site;
	char *ipv4;
	char *name;
	char aux[100];
	char status[10];
	char id[40];
	int real_size = 1000;

	*data = (char *) realloc(*data,real_size * sizeof(char));
	if(worker == NULL){
		printf("JSON_WORKER: Worker no existe\n");
		strcpy(*data,"300|{\"name\":\"WORKER no existe\"}");
		return 1;
	}
	
	strcpy(*data,"200|{\"name\":\"");
	strcat(*data,worker_get_name(worker));
	strcat(*data,"\",\"id\":\"");
	sprintf(aux,"%lu",worker_get_id(worker));
	strcat(*data,aux);
	strcat(*data,"\",\"ipv4\":\"");
	sprintf(aux,"%s",worker_get_ipv4(worker));
	strcat(*data,aux);
	strcat(*data,"\",\"real_status\":\"");
	itowstatus(worker_get_status(worker),status);
	sprintf(aux,"%s",status);
	strcat(*data,aux);

	printf("Llevamos de momento %s\n",*data);

	/* Obtenemos de la base de datos informacion relevante */
	if(!db_worker_get_info(db,worker_get_id(worker),aux)){
		printf("Fallo al querer obtener db\n");
		return 0;
	}
	strcat(*data,"\",");
	strcat(*data,aux);
	strcat(*data,"\",\"sites\":[ ");

	/* Poblamos con los sitios */
	lista_first(worker_get_sites(worker));
	while(!lista_eol(worker_get_sites(worker))){
		site = lista_get(worker_get_sites(worker));
		name = site_get_name(site);
		sprintf(id,"%lu",site_get_id(site));
		if(real_size < (strlen(name) + strlen(aux) + strlen(*data) + 100)){
			real_size += 1000;
			*data = (char *) realloc(*data,real_size * sizeof(char));
		}
		strcat(*data,"{\"id\":\"");
		sprintf(aux,"%s",id);
		strcat(*data,aux);
		strcat(*data,"\",\"name\":\"");
		strcat(*data,site_get_name(site));
		strcat(*data,"\"},");
		lista_next(worker_get_sites(worker));
	}
	*data = (char *) realloc(*data,(strlen(*data)+3) * sizeof(char));
	(*data)[strlen(*data)-1] = ']';
	strcat(*data,"}");
	return 1;
}

int json_proxy(char **data, T_proxy *proxy, T_db *db){
	/* Retorna todos los datos de un proxy
 	 * Realoca la memoria de data y retorna el tamano en
	 * size, si es que no entran los datos */

	T_site *site;
	char *ipv4;
	char *name;
	char aux[100];
	char status[10];
	int real_size = 1000;

	/* Por las dudas verificamos que data sea de al menos 1000 bytes */
	*data = (char *) realloc(*data, real_size * sizeof(char));
	
	if(proxy == NULL){
		strcpy(*data,"300|{\"name\":\"PROXY no existe\"}");
		return 1;
	}
	
	strcpy(*data,"200|{\"name\":\"");
	strcat(*data,proxy_get_name(proxy));
	strcat(*data,"\",\"id\":\"");
	sprintf(aux,"%lu",proxy_get_id(proxy));
	strcat(*data,aux);
	strcat(*data,"\",\"ipv4\":\"");
	sprintf(aux,"%s",proxy_get_ipv4(proxy));
	strcat(*data,aux);
	itopstatus(proxy_get_status(proxy),status);
	strcat(*data,"\",\"real_status\":\"");
	sprintf(aux,"\"%s\"",status);
	strcat(*data,aux);

	/* Obtenemos de la base de datos informacion relevante */
	if(!db_proxy_get_info(db,proxy_get_id(proxy),aux)){
		return 0;
	}
	strcat(*data,"\",");
	strcat(*data,aux);
	(*data)[strlen(*data)-1] = '}';
	*data = (char *) realloc(*data, (strlen(*data)+1) * sizeof(char));
	return 1;
}
