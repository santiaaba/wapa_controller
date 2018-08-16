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
	*data[strlen(*data)-1] = ']';
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
	
	strcpy(*data,"{\"name\":\"");
	strcat(*data,site_get_name(site));
	strcat(*data,"\",\"site_id\":\"");
	sprintf(aux,"%lu",site_get_id(site));
	strcat(*data,aux);
	strcat(*data,"\",\"user_id\":\"");
	sprintf(aux,"%lu",site_get_userid(site));
	strcat(*data,aux);
	strcat(*data,"\",\"suscrip_id\":\"");
	sprintf(aux,"%lu",site_get_susc(site));
	strcat(*data,aux);
	strcat(*data,"\",\"status\":\"");
	sprintf(aux,"%i",site_get_status(site));
	strcat(*data,aux);
	strcat(*data,"\",\"urls\":[");

	/* Poblamos con las urls */
	list_alias_first(site_get_alias(site));
	while(list_alias_eol(site_get_alias(site))){
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
	*data[strlen(*data)-1] = ']';
	strcat(*data,",\"workers\":[");

	/* Poblamos con los workers */
	list_worker_first(site_get_workers(site));
	while(list_worker_eol(site_get_workers(site))){
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
	*data[strlen(*data)-1] = ']';
	strcat(*data,"}");
}
