#include "json.h"

void json_sites(char **data, int *size, T_list_site *sites){
	/* Retorna los id de todos los sitios existentes */
	/* Realoca la memoria de data y retorna el tamano en
 	 * size, si es que no entran los datos */

	T_site *site;

	list_site_first(sites);
	strcpy(*data,"[");
	while(!list_site_eol(sites)){
		site = list_site_get(sites);
		if((40 + strlen(*data)) > *size){
			/* Los datos pueden superan el espacio de data */
			*size = *size + 100;
			*data = (char *) realloc(*data,*size);
		}
		sprintf(*data,"%s%lu,",*data,site_get_id(site));
		list_site_next(sites);
	}
	(*data)[strlen(*data)-1] = ']';
}

void json_site(char **data, int *size, T_site *site){
	/* Retorna todos los datos de un sitio
 	 * Realoca la memoria de data y retorna el tamano en
	 * size, si es que no entran los datos */

	T_alias *alias;
	T_worker *worker;
	int id;
	char *name;
	char aux[40];

	/* Por las dudas verificamos que data sea de al menos 1000 bytes */
	if(*size < 1000){
		*size = 1000;
		*data = (char *) realloc(*data,*size);
	}

	if(site == NULL){
		strcpy(*data,"{\"name\":\"SITE no existe\"}");
		return;
	}
	
	strcpy(*data,"{\"name\":\"");
	strcat(*data,site_get_name(site));
	strcat(*data,"\",\"site_id\":\"");
	sprintf(aux,"%lu",site_get_id(site));
	strcat(*data,aux);
	strcat(*data,"\",\"dir\":\"");
	sprintf(aux,"%s",site_get_dir(site));
	strcat(*data,aux);
	strcat(*data,"\",\"status\":\"");
	sprintf(aux,"%i",site_get_status(site));
	strcat(*data,aux);
	strcat(*data,"\",\"urls\":[ ");

	/* Poblamos con las urls */
	list_alias_first(site_get_alias(site));
	while(!list_alias_eol(site_get_alias(site))){
		alias = list_alias_get(site_get_alias(site));
		name = alias_get_name(alias);
		if(*size < (strlen(name) + 150)){
			*size = *size + 1000;
			*data = (char *) realloc(*data,*size);
		}
		strcat(*data,"{\"id\":\"");
		sprintf(aux,"%lu",alias_get_id(alias));
		strcat(*data,aux);
		strcat(*data,"\",\"url\":\"");
		strcat(*data,alias_get_name(alias));
		strcat(*data,"\"},");
		list_alias_next(site_get_alias(site));
	}
	(*data)[strlen(*data)-1] = ']';
	strcat(*data,",\"workers\":[ ");

	/* Poblamos con los workers */
	list_worker_first(site_get_workers(site));
	while(!list_worker_eol(site_get_workers(site))){
		worker = list_worker_get(site_get_workers(site));
		name = worker_get_name(worker);
		if(*size < (strlen(name) + 50)){
			*size = *size + 1000;
			*data = (char *) realloc(*data,*size);
		}
		strcat(*data,"{\"name\":\"");
		strcat(*data,worker_get_name(worker));
		strcat(*data,"\"},");
		list_worker_next(site_get_workers(site));
	}
	if(*size < (strlen(*data) + 3)){
		*size = *size + 3;
		*data = (char *) realloc(*data,*size);
	}
	(*data)[strlen(*data)-1] = ']';
	strcat(*data,"}");
}

void json_workers(char **data, int *size, T_list_worker *workers){
	/* Retorna los id de todos los workers existentes */
	/* Realoca la memoria de data y retorna el tamano en
 	 * size, si es que no entran los datos */

	T_worker *worker;

	list_worker_first(workers);
	strcpy(*data,"[");
	while(!list_worker_eol(workers)){
		worker = list_worker_get(workers);
		if((40 + strlen(*data)) > *size){
			/* Los datos pueden superan el espacio de data */
			*size = *size + 100;
			*data = (char *) realloc(*data,*size);
		}
		sprintf(*data,"%s%lu,",*data,worker_get_id(worker));
		list_worker_next(workers);
	}
	(*data)[strlen(*data)-1] = ']';
}


void json_worker(char **data, int *size, T_worker *worker){
	/* Retorna todos los datos de un worker
 	 * Realoca la memoria de data y retorna el tamano en
	 * size, si es que no entran los datos */

	T_site *site;
	char *ipv4;
	char *name;
	char aux[40];
	char id[40];

	/* Por las dudas verificamos que data sea de al menos 1000 bytes */
	if(*size < 1000){
		*size = 1000;
		*data = (char *) realloc(*data,*size);
	}
	
	if(worker == NULL){
		strcpy(*data,"{\"name\":\"WORKER no existe\"}");
		return;
	}
	
	strcpy(*data,"{\"name\":\"");
	strcat(*data,worker_get_name(worker));
	strcat(*data,"\",\"id\":\"");
	sprintf(aux,"%lu",worker_get_id(worker));
	strcat(*data,aux);
	strcat(*data,"\",\"ipv4\":\"");
	sprintf(aux,"%s",worker_get_ipv4(worker));
	strcat(*data,aux);
	strcat(*data,"\",\"status\":\"");
	sprintf(aux,"%i",worker_get_status(worker));
	strcat(*data,aux);
	strcat(*data,"\",\"sites\":[ ");

	/* Poblamos con los sitios */
	list_site_first(worker_get_sites(worker));
	while(!list_site_eol(worker_get_sites(worker))){
		site = list_site_get(worker_get_sites(worker));
		name = site_get_name(site);
		sprintf(id,"%lu",site_get_id(site));
		if(*size < (strlen(name) + strlen(aux) + strlen(*data) + 100)){
			*size = *size + 1000;
			*data = (char *) realloc(*data,*size);
		}
		strcat(*data,"{\"id\":\"");
		sprintf(aux,"%s",id);
		strcat(*data,aux);
		strcat(*data,"\",\"name\":\"");
		strcat(*data,site_get_name(site));
		strcat(*data,"\"},");
		list_site_next(worker_get_sites(worker));
	}
	if(*size < (strlen(*data) + 3)){
		*size = *size + 3;
		*data = (char *) realloc(*data,*size);
	}
	(*data)[strlen(*data)-1] = ']';
	strcat(*data,"}");
}
