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

int init_sync(T_list_worker *workers, T_list_site *sites){
	/* Al iniciar el controller, se encarga de sincronizar el mismo
	   con los workers. Esto es para no tener que redistribuir los
	   sitios que ya estan siendo atendidos por los workers que estan
	   online */

	T_worker *worker;

	list_worker_first(workers);
	while(!list_worker_eol(workers)){
		worker = list_worker_get(workers);
		printf("revisando worker %s\n", worker_get_name(worker));
		worker_sync(worker,sites);
		printf("Pasamos de worker------\n");
		list_worker_next(workers);
	}
	return 1;
}

int check_workers(T_list_worker *workers){
	/* Verifica el estado de un worker y en base al mismo toma acciones. */
	
	T_worker *worker;

	list_worker_first(workers);
	while(!list_worker_eol(workers)){
		worker = list_worker_get(workers);
		worker_check(worker);
		if(worker_get_status(worker)){
			printf("Implementar\n");
		}
		list_worker_next(workers);
	}
	return 1;
}

int select_workers(T_list_worker *candidates, T_site *site){
	/* Selecciona los workers candidatos para asignar un sitio
	 * que necesite n cantidad de ellos */

	return 1;
}

int assign_workers(T_list_worker *candidates, T_site *site){
	/* Asigna un sitio a cada worker de la lista pasada
	 * por parametro */

	T_worker *worker;

	list_worker_first(candidates);
	while(!list_worker_eol(candidates)){
		worker = list_worker_get(candidates);
		worker_add_site(worker,site);
		list_worker_next(candidates);
	}
	return 1;
}

int reasign_sites(T_worker *w_fail){
	/* Dado el worker wfail que esta fallando, se redistribuyen
	 * los sitios entre los demas workers */

	T_list_worker candidates;

	list_worker_init(&candidates);
	list_site_first(worker_get_sites(w_fail));
	while(!list_site_eol(worker_get_sites(w_fail))){
		select_workers(&candidates,list_site_get(worker_get_sites(w_fail)));
		assign_workers(&candidates,list_site_get(worker_get_sites(w_fail)));
	}
	return 1;
}

int rebalance(){
	/* Cuando un worker queda fuera de servicio o entra en servicio
	 * debemos rebalancear los sitios siempre y cuando los workers
	 * presenten niveles de carga elevados. Determinar que es elevado */

	return 1;
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
	init_sync(&workers,&sites);

	/* Reconfiguramos los proxys */

	/* Iniciamos el server REST para la API */

	/* Comenzamos el loop del controller */
	while(1){
		/* Chequeo de workers */
		//check_workers(&workers);
		/* Chequeo de proxys */
	}

	/* Finalizamos la conexion a la base */
	db_close(&db);
}
