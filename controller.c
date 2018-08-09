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

int balance_workers(T_list_worker *workers){
	/* A implementar */
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
		
		printf("\tSolicitamos check al worker %s\n",worker_get_name(worker));
		if(worker_check(worker)){
			printf("\tworker cambio de estado\n");
			itowstatus(worker_get_status(worker),status);
			itowstatus(worker_get_last_status(worker),last_status);
			printf("\tlast_status %s -> status %s\n",last_status,status);
			if((worker_get_status(worker) == W_PREPARED)){
				/* Worker se recupera de una falla */
				worker_purge(worker);
			}
			if((worker_get_status(worker) == W_ONLINE) && (worker_get_last_status(worker) == W_PREPARED)){
				printf("\tWorker %s paso de PREPARED a ONLINE.\n",worker_get_name(worker));
				/* Debemos balancear posiblemente */
				balance_workers(workers);
			}
			if((worker_get_status(worker) == W_BROKEN ||
			   worker_get_status(worker) == W_UNKNOWN) &&
			   worker_get_last_status(worker) == W_ONLINE){
				printf("\tWorker %s paso de ONLINE a FALLANDO.\n",worker_get_name(worker));
				/* Worker estaba online y ha sufrido un fallo */
				worker_purge(worker);
				/* El proceso normalice_sites se encargara de asignar los sitios
 				 * que se han eliminado del worker con falla */
			}
		}

		list_worker_next(workers);
	}
	printf("--FIN: Proceso check_workers\n");
	return 1;
}

int select_workers(T_list_worker *workers, T_list_worker *candidates, T_site *site){
	/* Selecciona los workers candidatos para asignar un sitio
	 * que necesite n cantidad de ellos */
	/* La condicion de momento es workers activos con menor carga (load average) */

	T_worker *worker;
	char aux[50];

	list_worker_sort_by_load(workers,0);
	list_worker_first(workers);
	while(site_get_real_size(site) < site_get_size(site)
	&& !list_worker_eol(workers)){
		worker = list_worker_get(workers);
		printf("Verificamos worker para candidato\n");
		printf("	Name: %s\n",worker_get_name(worker));
		itowstatus(worker_get_status(worker),aux);
		printf("	Status: %s\n",aux);
		printf("	id: %i\n",worker_get_id(worker));
		if((worker_get_status(worker) == W_ONLINE)
		&& (!list_worker_find_id(site_get_workers(site),worker_get_id(worker)))){
			printf("Worker %s es candidato\n",worker_get_name(worker));
			list_worker_add(candidates,list_worker_get(workers));
		}
		list_worker_next(workers);
	}
	return 1;
}

int assign_workers(T_list_worker *candidates, T_site *site, T_config *config){
	/* Asigna un sitio a cada worker de la lista pasada
	 * por parametro */

	T_worker *worker;

	printf("Comenzamos a asignar workers\n");
	list_worker_first(candidates);
	while(!list_worker_eol(candidates)){
		worker = list_worker_get(candidates);
		worker_add_site(worker,site,config_default_domain(config));
		list_worker_next(candidates);
	}
	return 1;
}

void des_assign_workers(T_site *site){
	/* Quita workers de un sitio en el cual excede su cantidad
	Los workers que se eliminan son los que tengan mayor carga (load average) */

	list_worker_sort_by_load(site_get_workers(site),1);
	while(site_get_real_size(site) > site_get_size(site)){
		list_worker_remove(site_get_workers(site));
	}
}

int normalice_sites(T_list_site *sites, T_list_worker *workers, T_config *config){
	/* Recorre la lista de sitios buscando sitios donde la
	 * cantidad de workers asignados no sea la adecuada */

	T_site *site;
	T_list_worker candidates;
	int siterealsize;

	printf("\n----- NORMALICE ----\n");
	list_site_first(sites);
	list_worker_init(&candidates);
	while(!list_site_eol(sites)){
		site = list_site_get(sites);
		siterealsize = site_get_real_size(site);
		printf("\tRevisamos el sitio %s - real %i: need %i\n",site_get_name(site),siterealsize,site_get_size(site));
		if(siterealsize < site_get_size(site)){
			/* Estan faltando workers */
			printf("\tBorramos posibles workers en lista candidatos\n");
			list_worker_erase(&candidates);
			printf("\tSeleccionamos los workers\n");
			select_workers(workers,&candidates,site);
			printf("\tAsignamos los workers\n");
			assign_workers(&candidates,site,config);
		} else {
			if(siterealsize > site_get_size(site)){
				/* Estan sobrando workers */
				printf("\tEstan sobrando workers\n");
				des_assign_workers(site);
			}
		}
		list_site_next(sites);
	}
	printf("\n----- FIN NORMALICE ----\n");
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

	/* Imprimimos la lista de sitios */
	list_site_first(&sites);
	while(!list_site_eol(&sites)){
		printf("sitio cargado %s\n", site_get_name(list_site_get(&sites)));
		list_site_next(&sites);
	}

	/* Sincronizamos con la información en workers y proxys */
	printf("--INI Sincronizamos workers--\n");
	init_sync(&workers,&sites);

	printf("--FIN Sincronizamos workers--\n");

	/* Reconfiguramos los proxys */

	/* Iniciamos el server REST para la API */

	/* Comenzamos el loop del controller */
	while(1){
		printf("Entra al LOOP\n");
		/* Chequeo de workers */
		sleep(5);
		check_workers(&workers);
		
		/* Chequeo de proxys */
		//check_proxys(&proxys);
		
		/* Asignacion sitios a worker */
		//normalice_sites(&sites, &workers, &config);
		
	}

	/* Finalizamos la conexion a la base */
	db_close(&db);
}
