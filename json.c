#include "json.h"

void json_servers(char **data, T_list_worker *workers, T_list_proxy *proxys, T_db *db){
	/* Retorna los id de todos los workers y proxys existentes y alguna otra info relevante */
	/* Realoca la memoria de data y retorna el tamano en size, si es que no entran los datos */

	T_worker *worker;
	T_proxy *proxy;
	char status[20];
	int real_size = 100;

	*data = (char *) realloc(*data,real_size * sizeof(char));
	strcpy(*data,"200|[");
	list_worker_first(workers);
	while(!list_worker_eol(workers)){
		worker = list_worker_get(workers);
		if((100 + strlen(*data)) > real_size){
			/* Los datos pueden superan el espacio de data */
			real_size += 100;
			*data = (char *) realloc(*data,real_size * sizeof(char));
		}
		itowstatus(worker_get_status(worker),status);
		sprintf(*data,"%s{\"id\":%lu,\"name\":\"%s\",\"rol\":\"worker\",\"status\":\"%s\"},",
			*data,worker_get_id(worker),worker_get_name(worker),status);
		list_worker_next(workers);
	}
	list_proxy_first(proxys);
	while(!list_proxy_eol(proxys)){
		proxy = list_proxy_get(proxys);
		if((100 + strlen(*data)) > real_size){
			/* Los datos pueden superan el espacio de data */
			real_size += 100;
			*data = (char *) realloc(*data,real_size * sizeof(char));
		}
		itopstatus(proxy_get_status(proxy),status);
		sprintf(*data,"%s{\"id\":%lu,\"name\":\"%s\",\"rol\":\"proxy\",\"status\":\"%s\"},",
			*data,proxy_get_id(proxy),proxy_get_name(proxy),status);
		list_proxy_next(proxys);
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
	list_site_first(worker_get_sites(worker));
	while(!list_site_eol(worker_get_sites(worker))){
		site = list_site_get(worker_get_sites(worker));
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
		list_site_next(worker_get_sites(worker));
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
