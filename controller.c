#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "structs.h"
#include "db.h"
#include "config.h"
#include <pthread.h>
#include "rest_server.h"

/********************************
 * 	Variables GLOBALES	*
 ********************************/
T_config config;
T_list_worker workers;
T_list_proxy proxys;
T_list_site sites;
T_db db;
T_rest_server rest_server;

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

int balance_workers(T_list_worker *workers, T_config *config){

	/* Se basa en que todos los workers son compartidos.
 	 * Determina que worker esta sobre cargado e intenta
 	 * mover sitios de el a uno con menor carga y que ya no
 	 * contegan a dicho sitio.*/

	 /* Retorna 1 si ha habido cambios. Caso contrario 0 */
	
	T_worker *wfirst = NULL;
	T_worker *wlast = NULL;
	T_site *site;
	int encontro=0;

	printf("---- BALANCE -----\n");

	list_worker_sort_by_site(workers,1);
	//list_worker_sort_by_load(workers,1);
	
	list_worker_first(workers);
	while(!list_worker_eol(workers)){
		if(worker_get_status(list_worker_get(workers)) == W_ONLINE)
			wlast = list_worker_get(workers);
		list_worker_next(workers);
	}
	/*Buscamos mas cargado activo */
	list_worker_first(workers);
	while((wfirst == NULL) && !list_worker_eol(workers)){
		if(worker_get_status(list_worker_get(workers)) == W_ONLINE)
			wfirst = list_worker_get(workers);
		list_worker_next(workers);
	}

	if((wfirst != wlast) && (wfirst!=NULL)){
		printf("wfirst: %i - wlast: %i\n",list_site_size(worker_get_sites(wfirst)),list_site_size(worker_get_sites(wlast)));
		//if(worker_get_load(wfirst) > worker_get_load(wlast) * (1+config_load_average(config)/100)){
		if((list_site_size(worker_get_sites(wfirst))) > (list_site_size(worker_get_sites(wlast))+1)){
			/* Desbalaceo de carga. Buscamos el sotio a mover */
			list_site_first(worker_get_sites(wfirst));
			while(!encontro && !list_site_eol(worker_get_sites(wfirst))){
				site = list_site_get(worker_get_sites(wfirst));
				/* Verificamos que el sitio no este ya en el worker
				 * de destino */
				if(!list_worker_find_id(site_get_workers(site),worker_get_id(wlast))){
					/* El sitio no se encuentra en el worker de destino
 					   lo movemos */
					worker_remove_site(wfirst,site);
					worker_add_site(wlast, site,config_default_domain(config));
					encontro=1;
				}
			}
		}
		printf("---- FIN BALANCE -----\n");
		return encontro;
	} else 
		return 0;
}

void migrate_sites(T_worker *worker, T_list_worker *aux_worker_list,
		   T_list_proxy *proxys, T_config *config){
	/* Migra los sitios de un worker a otros */

	T_site *site;
	T_list_worker candidates;

	printf("------ MIGRATE START -----\n");
	list_worker_init(&candidates);
	list_site_first(worker_get_sites(worker));
	while(!list_site_eol(worker_get_sites(worker))){
		list_worker_erase(&candidates);
		site = list_site_get(worker_get_sites(worker));
		printf("migramos sitio %s - %i\n",site_get_name(site),list_worker_size(aux_worker_list));
		REVISANDO PORQUE NO MIGRA
		select_workers(aux_worker_list,&candidates,site);
		assign_workers(&candidates,proxys,site,config);
		list_site_next(worker_get_sites(worker));
	}
	list_worker_erase(&candidates);
	printf("------ MIGRATE END -----\n");
}

int reload_services(T_list_worker *workers, T_list_proxy *proxys){

	list_worker_first(workers);
	while(!list_worker_eol(workers)){
		worker_reload(list_worker_get(workers));
		list_worker_next(workers);
	}
	list_proxy_first(proxys);
	while(!list_proxy_eol(proxys)){
		proxy_reload(list_proxy_get(proxys));
		list_proxy_next(proxys);
	}
}

int check_proxys(T_list_proxy *proxys, T_list_site *sites){

	T_proxy *proxy;

	list_proxy_first(proxys);
	while(!list_proxy_eol(proxys)){
		proxy = list_proxy_get(proxys);
		printf("Chequeando proxy %s\n",proxy_get_name(proxy));
		if(proxy_check(proxy)){
			printf("Proxy cambio de estado\n");
			if((proxy_get_status(proxy) == W_PREPARED)){
				/* Reconfiguramos el proxy */
				proxy_reconfig(proxy,sites);
				proxy_reload(proxy);
			}
			if((proxy_get_status(proxy) == W_ONLINE) &&
			(proxy_get_last_status(proxy) == W_PREPARED)){
				printf("\tProxy %s paso de PREPARED a ONLINE.\n",
					proxy_get_name(proxy));
			}
		}
		list_proxy_next(proxys);
	}
}

int check_workers(T_list_worker *workers, T_list_proxy *proxys, T_config *config){
	/* Verifica el estado de un worker y en base al mismo toma acciones. */
	
	printf("\n--INI: Proceso check_workers\n");
	T_worker *worker;
	T_list_worker aux_worker_list;
	char status[20];
	char last_status[20];

	list_worker_first(workers);
	printf("Cantidad Workers: %i\n",list_worker_size(workers));
	while(!list_worker_eol(workers)){
		worker = list_worker_get(workers);
		
		printf("\tSolicitamos check al worker %s\n",worker_get_name(worker));
		if(worker_check(worker)){
			printf("\tworker cambio de estado\n");
			itowstatus(worker_get_status(worker),status);
			itowstatus(worker_get_last_status(worker),last_status);
			printf("\tlast_status %s -> status %s\n",last_status,status);
			if((worker_get_status(worker) == W_OFFLINE)){
				/* Worker pasa a OFFLINE. Despoblamos de sitios */
				list_worker_init(&aux_worker_list);
				list_worker_copy(workers,&aux_worker_list);
				migrate_sites(worker,&aux_worker_list,proxys,config);
				list_worker_erase(&aux_worker_list);
			}
			if((worker_get_status(worker) == W_PREPARED)){
				/* Worker se recupera de una falla */
				worker_purge(worker);
			}
			if((worker_get_status(worker) == W_ONLINE) &&
			   (worker_get_last_status(worker) == W_PREPARED)){
				printf("\tWorker %s paso de PREPARED a ONLINE.\n",
					worker_get_name(worker));
				/* Debemos balancear posiblemente */
				//balance_workers(workers);
			}
			if((worker_get_status(worker) == W_BROKEN ||
			   worker_get_status(worker) == W_UNKNOWN) &&
			   worker_get_last_status(worker) == W_ONLINE){
				printf("\tWorker %s paso de ONLINE a estado FALLANDO.\n",
					worker_get_name(worker));
				/* Worker estaba online y ha sufrido un fallo */
				worker_purge(worker);
				/* El proceso normalice_sites se encargara de asignar
				   los sitios que se han eliminado del worker con falla */
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
	/* Retorna la cantidad de workers seleccionados */

	T_worker *worker;
	char aux[50];
	int cant=0;

	printf("Seleccionamos los workers\n");
	/* PAra pruebas utilizamos ort antidad de sitios */
	list_worker_sort_by_site(workers,0);
	//list_worker_sort_by_load(workers,0);
	list_worker_first(workers);
	while((site_get_real_size(site) + cant) < site_get_size(site)
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
			cant++;
		}
		list_worker_next(workers);
	}
	return list_worker_size(candidates);
}

int assign_workers(T_list_worker *candidates, T_list_proxy *proxys,
		   T_site *site, T_config *config){
	/* Asigna un sitio a cada worker de la lista pasada
	 * por parametro. Retorna la cantidad de workers donde se pudo
	 * asignar */

	T_worker *worker;
	T_proxy *proxy;
	int cant=0;

	printf("Comenzamos a asignar workers\n");
	list_worker_first(candidates);
	/* Agregamos el sitio a los workers */
	while(!list_worker_eol(candidates)){
		worker = list_worker_get(candidates);
		if(worker_add_site(worker,site,config_default_domain(config)))
			cant++;
		list_worker_next(candidates);
	}
	/* Actualizamos el sitio en los proxys */
	list_proxy_first(proxys);
	while(!list_proxy_eol(proxys)){
		proxy = list_proxy_get(proxys);
		proxy_change_site(proxy,site);
		list_proxy_next(proxys);
	}
	return cant;
}

int des_assign_workers(T_site *site, T_list_worker *workers){
	/* Quita workers de un sitio en el cual excede su cantidad
	Los workers que se eliminan son los que tengan mayor carga (load average) */

	int cant=0;
	T_worker *worker;

	list_worker_sort_by_load(site_get_workers(site),1);
	while(site_get_real_size(site) > site_get_size(site)){
		worker = list_worker_remove(site_get_workers(site));
		if(worker){
			cant++;
			/* Removemos ahora del worker el sitio*/
			list_site_remove_id(worker_get_sites(worker),site_get_id(site));
		}
	}
	return cant;
}

int normalice_sites(T_list_site *sites, T_list_worker *workers,
		    T_list_proxy *proxys, T_config *config){
	/* Recorre la lista de sitios buscando sitios donde la
	 * cantidad de workers asignados no sea la adecuada */

	/* Retorna 1 si al menos se a producido un cambio */

	T_site *site;
	T_list_worker candidates;
	int siterealsize;
	int changed = 0;

	printf("\n----- NORMALICE ----\n");
	list_site_print(sites);
	list_site_first(sites);
	list_worker_init(&candidates);
	while(!list_site_eol(sites)){
		site = list_site_get(sites);
		siterealsize = site_get_real_size(site);
		printf("\tRevisamos el sitio %s - real %i: need %i\n",site_get_name(site),siterealsize,site_get_size(site));
		if(siterealsize < site_get_size(site)){
			/* Estan faltando workers */
			list_worker_erase(&candidates);
			if(select_workers(workers,&candidates,site)){
				printf("\tAsignamos los workers\n");
				changed |= assign_workers(&candidates,proxys,site,config);
			}
		} else {
			if(siterealsize > site_get_size(site)){
				/* Estan sobrando workers */
				printf("\tEstan sobrando workers\n");
				changed |= des_assign_workers(site,workers);
			}
		}
		list_site_next(sites);
	}
	list_worker_erase(&candidates);
	printf("\n----- FIN NORMALICE ----\n");
	return changed;
}

/********************************
 * 		MAIN		*
 ********************************/
void main(){

	int changed;

	srand(time(NULL));

	/* Cargamos la configuracion */
	config_load("controller.conf",&config);

	printf("db_server : %s\n",config_db_server(&config));
	printf("db_user : %s\n",config_db_user(&config));
	printf("db_pass : %s\n",config_db_pass(&config));
	printf("db_name : %s\n",config_db_name(&config));
	printf("load_average : %i\n",config_load_average(&config));
	printf("site_average : %i\n",config_sites_average(&config));

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

	/* Iniciamos el server REST para la API */
	rest_server_init(&rest_server,&sites,&workers,&proxys,&db);
	printf("El rest server posee puntero %p\n",rest_server);

	/* Comenzamos el loop del controller */
	while(1){
		changed = 0;
		rest_server_lock(&rest_server);

		/* Chequeo de workers */
		check_workers(&workers,&proxys,&config);
		/* Chequeo de proxys */
		check_proxys(&proxys,&sites);
		
		/* Asignacion sitios a worker */
		changed |= normalice_sites(&sites, &workers, &proxys, &config);

		/* Balanceamos workers */
		changed |= balance_workers(&workers,&config);
		rest_server_unlock(&rest_server);

		if(changed)
			reload_services(&workers,&proxys);

		sleep(5);
		printf("Fin del bucle");
	}

	/* Finalizamos el hilo del rest_server */

	/* Finalizamos la conexion a la base */
	db_close(&db);
}
