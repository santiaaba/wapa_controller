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
	   con los workers. Esto es... se obtiene de cada worker fisico W_ONLINE
	   los sitios a los que responde y se cargan en el worker logico.
	   */

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
	
	printf("\n--INI: Proceso check_workers\n");
	T_worker *worker;
	char status[20];
	char last_status[20];

	list_worker_first(workers);
	while(!list_worker_eol(workers)){
		worker = list_worker_get(workers);
		printf("Solicitamos check al worker\n");
		worker_check(worker);
		printf("comparamos resultado obtenido\n");
		itowstatus(worker_get_status(worker),status);
		itowstatus(worker_get_last_status(worker),last_status);
		printf("status %s - last_status %s\n",status,last_status);
		if(worker_get_status(worker) == W_ONLINE){
			if(worker_get_last_status(worker) == W_PREPARED){
				/* Worker se recupera de una falla */
				printf("Worker %s paso de PREPARED a ONLINE. Requiere balance\n",worker_get_name(worker));
				//balance(workers);
			}
		}
		if((worker_get_status(worker) == W_BROKEN ||
		   worker_get_status(worker) == W_UNKNOWN) &&
		   worker_get_last_status(worker) == W_ONLINE){
			/* Worker estaba online y ha sufrido un fallo */
			/* Se le quitan los sitios */
			printf("Worker %s paso a BROKEN o UNKNOWN. Requiere balance\n",worker_get_name(worker));
			//worker_purge(worker);
			//balance(workers);
		}
		list_worker_next(workers);
	}
	printf("--FIN: Proceso check_workers\n");
	return 1;
}

int select_workers(T_list_worker *workers, T_list_worker *candidates, T_site *site){
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

int des_assign_workers(T_site *site){
	return 1;
}

int normalice_sites(T_list_site *sites, T_list_worker *workers){
	/* Recorre la lista de sitios buscando sitios donde la
	 * cantidad de workers asignados no sea la adecuada */

	T_site *site;
	T_list_worker candidates;
	int siterealsize;

	list_site_first(sites);
	list_worker_init(&candidates);
	while(!list_site_eol(sites)){
		site = list_site_get(sites);
		siterealsize = site_get_real_size(site);
		if(siterealsize > site_get_size(site)){
			/* Estan faltando workers */
			list_worker_erase(&candidates);
			select_workers(workers,&candidates,site);
			assign_workers(&candidates,site);
		} else {
			if(siterealsize < site_get_size(site)){
				/* Estan sobrando workers */
				des_assign_workers(site);
			}
		}
		list_site_next(sites);
	}
	return 1;
	
}

int balance(T_list_worker *workers){
	/* Cuando un worker queda fuera de servicio o entra en servicio
	 * debemos rebalancear los sitios. OJO que puede generar afectación. */
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
	printf("--INI Sincronizamos workers--\n");
	init_sync(&workers,&sites);
	printf("--FIN Sincronizamos workers--\n");

	/* Reconfiguramos los proxys */

	/* Iniciamos el server REST para la API */

	/* Comenzamos el loop del controller */
	while(1){
		/* Chequeo de workers */
		check_workers(&workers);
		
		/* Chequeo de proxys */
		//check_proxys(&proxys);
		
		/* Asignacion sitios a worker */
		//assign_sites(sites,workers);
		
		sleep(5);
	}

	/* Finalizamos la conexion a la base */
	db_close(&db);
}
