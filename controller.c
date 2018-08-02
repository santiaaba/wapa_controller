#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "structs.h"
#include "db.h"
#include "config.h"

/********************************
 * 	Variables GLOBALES	*
 ********************************/
T_config config;
T_list_worker workers;
T_list_proxy proxys;
T_list_site sites;
T_db db;

/********************************
 * 	FUNCIONES		*
 ********************************/

int init_sync(){
	/* Al iniciar el controller, se encarga de sincronizar el mismo
	   con los workers. Esto es para no tener que redistribuir los
	   sitios que ya estan siendo atendidos por los workers que estan
	   online */

	T_worker *worker;

	list_worker_first(workers);
	while(!list_worker_eol(workers)){
		worker = list_worker_get(workers);
		worker_sync(w);
		list_worker_next(workers);
	}
}

int check_worker(T_worker *w){
	/* Verifica el estado de un worker y en base al mismo toma acciones. */
	
	T_worker *worker;

	list_worker_first(workers);
	while(!list_worker_eol(workers)){
		worker = list_worker_get(workers);
		worker_check(w);
		if(worker_get_status(w)){
		}
		list_worker_next(workers);
	}
}

/********************************
 * 		MAIN		*
 ********************************/
void main(){

	/* Cargamos la configuracion */
	config_load("controller.conf",&config);

	printf("db_server : %s\n",config_db_server(&config));
	printf("db_user : %s\n",config_db_user(&config));
	printf("db_pass : %s\n",config_db_pass(&config));
	printf("db_name : %s\n",config_db_name(&config));

	/* Iniciamos estructuras */
	list_worker_init(&workers);
	list_site_init(&sites);
	list_proxy_init(&proxys);

	/* Conectamos contra la base de datos */
	db_init(&db);
	if (!db_connect(&db,&config)){
		printf("Error coneccion a la base de datos: %s\n",db_error(&db));
		exit(1);
	}
	/* Cargamos los datos de la base de datos */
	db_load_sites(&db,&sites);
	db_load_workers(&db,&workers);
	db_load_proxys(&db,&proxys);

	/* Sincronizamos con la información en workers y proxys */
	init_sync();

	/* Reconfiguramos los proxys */

	/* Iniciamos el server REST para la API */

	/* Comenzamos el loop del controller */
	while(1){
		/* Chequeo de workers */
		/* Chequeo de proxys */
	}

	/* Finalizamos la conexion a la base */
	db_close(&db);
}
