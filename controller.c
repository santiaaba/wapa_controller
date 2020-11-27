#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "structs.h"
#include "db.h"
#include "logs.h"
#include "config.h"
#include "server.h"
#include "lista.h"

/********************************
 * 	Variables GLOBALES	*
 ********************************/
T_config config;
T_logs logs;
T_lista workers;
T_lista proxys;
T_lista sites;
T_db db;
T_server server;

/********************************
 * 	FUNCIONES		*
 ********************************/

void assign_proxys_site(T_lista *proxys, T_site *site){
	/* Actualizamos el sitio en los proxys */
	T_proxy *proxy;

	lista_first(proxys);
	while(!lista_eol(proxys)){
		proxy = lista_get(proxys);
		proxy_change_site(proxy,site);
		lista_next(proxys);
	}
}

void assign_proxys_sites(T_lista *proxys, T_lista *sites){
	/* Actualizamos todos los sitios del listado en los proxys */
	T_proxy *proxy;
	T_site *site;

	lista_first(sites);
	while(!lista_eol(sites)){
		site = lista_get(sites);
		lista_first(proxys);
		while(!lista_eol(proxys)){
			proxy = lista_get(proxys);
			if(proxy_get_status(proxy) == P_ONLINE ||
			   proxy_get_status(proxy) == P_PREPARED){
				printf("assing_proxys_site: Actualizando sitio %s en proxy %s\n",
						site_get_name(site),proxy_get_name(proxy));
				proxy_change_site(proxy,site);
			}
			lista_next(proxys);
		}
		lista_next(sites);
	}
}

void init_sync(T_lista *workers, T_lista *sites, T_lista *proxys, T_logs *logs){
	/* Al iniciar el controller, se encarga de sincronizar el mismo
	   con los workers. Esto es... se obtiene de cada worker fisico W_ONLINE
	   los sitios a los que responde y se cargan en el worker logico.
	   */

	T_worker *worker;

	logs_write(logs,L_DEBUG,"INIT_SYNC:","Iniciando sincronismo");
	lista_first(workers);
	while(!lista_eol(workers)){
		worker = lista_get(workers);
		worker_sync(worker,sites);
		lista_next(workers);
	}
	/* Reconfiguramos todos los proxys */
	assign_proxys_sites(proxys,sites);
	logs_write(logs,L_DEBUG,"INIT_SYNC:","Fin sincronismo");
}

int select_workers(T_lista *workers, T_lista *candidates, T_site *site){
	/* Selecciona los workers candidatos para asignar un sitio
	 * que necesite n cantidad de ellos */
	/* La condicion de momento es workers activos con menor carga (load average) */
	/* Retorna la cantidad de workers seleccionados */

	T_worker *worker;
	char aux[50];
	int cant=0;

	printf("Seleccionamos los workers\n");
	/* Para pruebas ordenamos por cantidad de sitios */
	lista_sort(workers,worker_num_sites,0);
	//lista_sort_by_load(workers,0);
	lista_first(workers);
	while((site_get_real_size(site) + cant) < site_get_size(site)
	     && !lista_eol(workers)){
		worker = lista_get(workers);
		printf("Verificamos worker para candidato\n");
		printf("	Name: %s\n",worker_get_name(worker));
		itowstatus(worker_get_status(worker),aux);
		printf("	Status: %s\n",aux);
		printf("	id: %i\n",worker_get_id(worker));
		if((worker_get_status(worker) == W_ONLINE)
		&& (!lista_find(site_get_workers(site),worker_get_id,worker_get_id(worker)))){
			printf("Worker %s es candidato\n",worker_get_name(worker));
			lista_add(candidates,lista_get(workers));
			cant++;
		}
		lista_next(workers);
	}
	return lista_size(candidates);
}

int assign_workers(T_lista *candidates, T_lista *proxys,
		   T_site *site, T_config *config){
	/* Asigna un sitio a cada worker de la lista pasada
	 * por parametro. Retorna la cantidad de workers donde se pudo
	 * asignar */

	T_worker *worker;
	int cant=0;

	printf("Comenzamos a asignar workers\n");
	lista_first(candidates);
	/* Agregamos el sitio a los workers */
	while(!lista_eol(candidates)){
		worker = lista_get(candidates);
		if(worker_add_site(worker,site))
			cant++;
		lista_next(candidates);
	}
	assign_proxys_site(proxys,site);
	return cant;
}

int des_assign_workers(T_site *site, T_lista *workers){
	/* Quita workers de un sitio en el cual excede su cantidad o porque
	 * paso a estar OFFLINE. Los workers que se eliminan son los que
	 * tengan mayor carga (load average) */

	int cant=0;
	char *aux = NULL;
	int size;	/* Cantidad de workers que debe tener el sitio asignados */
	T_worker *worker;
	if(site_get_status(site) == S_OFFLINE)
		size = 0;
	else
		size = site_get_size(site);
	printf("Size: %i contra %i\n",size,site_get_real_size(site));

	lista_sort(site_get_workers(site),worker_get_load,1);
	while(site_get_real_size(site) > size){
		lista_first(site_get_workers(site));
		worker = (T_worker *)lista_remove(site_get_workers(site));
		printf("El worker es: %p\n",worker);
		dim_init(&aux);
		printf("Hasta aca llegamos %s\n", worker_get_name(worker));
		worker_to_json(worker,&aux);
		printf("Removemos un worker: %s\n",aux);
		if(worker){
			cant++;
			/* Removemos ahora del worker el sitio*/
			printf("Removemos el worker del sitio %s\n", worker_get_name(worker));
			lista_exclude(worker_get_sites(worker),site_get_id,site_get_id(site));
		}
	}
	return cant;
}

int balance_workers(T_lista *workers, T_lista *proxys, T_config *config){

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

	lista_sort(workers,worker_num_sites,1);
	//lista_sort_by_load(workers,1);
	
	/* Buscamos el worker menos cargado activo */
	lista_first(workers);
	while(!lista_eol(workers)){
		if(worker_get_status(lista_get(workers)) == W_ONLINE)
			wlast = lista_get(workers);
		lista_next(workers);
	}

	/*Buscamos el worker mas cargado activo */
	lista_first(workers);
	while((wfirst == NULL) && !lista_eol(workers)){
		if(worker_get_status(lista_get(workers)) == W_ONLINE)
			wfirst = lista_get(workers);
		lista_next(workers);
	}

	if((wfirst != wlast) && (wfirst!=NULL)){
		printf("wfirst: %i - wlast: %i\n",lista_size(worker_get_sites(wfirst)),lista_size(worker_get_sites(wlast)));
		//if(worker_get_load(wfirst) > worker_get_load(wlast) * (1+config_load_average(config)/100)){
		if((lista_size(worker_get_sites(wfirst))) > (lista_size(worker_get_sites(wlast))+1)){
			/* Desbalaceo de carga. Buscamos el sitio a mover */
			/* De momento movemos el primero que no se encuentre en el worker de destino.
  			 * A futuro podría ser el que mas demanda procesador */
			lista_first(worker_get_sites(wfirst));
			while(!encontro && !lista_eol(worker_get_sites(wfirst))){
				site = lista_get(worker_get_sites(wfirst));
				/* Verificamos que el sitio no este ya en el worker
				 * de destino */
				if(!lista_find(site_get_workers(site),worker_get_id,worker_get_id(wlast))){
					/* El sitio no se encuentra en el worker de destino
 					   lo movemos */
					worker_remove_site(wfirst,site);
					worker_add_site(wlast, site);
					assign_proxys_site(proxys,site);
					encontro=1;
				}
			}
		}
		printf("---- FIN BALANCE -----\n");
		return encontro;
	} else 
		return 0;
}

int reload_services(T_lista *workers, T_lista *proxys){

	lista_first(workers);
	while(!lista_eol(workers)){
		worker_reload(lista_get(workers));
		lista_next(workers);
	}
	lista_first(proxys);
	while(!lista_eol(proxys)){
		proxy_reload(lista_get(proxys));
		lista_next(proxys);
	}
}

void check_db(T_db *db){
	// Verifica el estado de la conexion a la base de datos
	printf("Chequeamos base\n");
	if(!db_live(db)){
		printf("DB NO CONNECT!!!!");
		db_connect(db);
	}
	printf("Termina chequeo DB\n");
}

int check_proxys(T_lista *proxys, T_lista *sites){

	T_proxy *proxy;
	int cambio = 0;

	lista_first(proxys);
	while(!lista_eol(proxys)){
		proxy = lista_get(proxys);
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
			cambio=1;
		}
		lista_next(proxys);
	}
	return cambio;
}

int check_workers(T_lista *workers, T_lista *proxys, T_config *config){
	/* Verifica el estado de un worker y en base al mismo toma acciones. */
	/* Retorna 1 si hubo cambios */
	
	printf("\n--INI: Proceso check_workers\n");
	T_worker *worker;
	T_lista aux_sites;
	char status[20];
	int cambio = 0;
	char last_status[20];

	lista_first(workers);
	printf("Cantidad Workers: %i\n",lista_size(workers));
	while(!lista_eol(workers)){
		worker = lista_get(workers);
		
		printf("\tSolicitamos check al worker %s\n",worker_get_name(worker));
		if(worker_check(worker)){
			printf("\tworker cambio de estado\n");
			itowstatus(worker_get_status(worker),status);
			itowstatus(worker_get_last_status(worker),last_status);
			printf("\tlast_status %s -> status %s\n",last_status,status);
			lista_init(&aux_sites,sizeof(T_site));
			if((worker_get_status(worker) == W_OFFLINE)){
				/* Worker pasa a OFFLINE. Despoblamos de sitios */
				lista_copy(worker_get_sites(worker),&aux_sites);
				worker_purge(worker);
				assign_proxys_sites(proxys,&aux_sites);
				lista_erase(&aux_sites);
			}
			if((worker_get_status(worker) == W_PREPARED)){
				/* Worker se recupera de una falla */
				lista_copy(worker_get_sites(worker),&aux_sites);
				worker_purge(worker);
				assign_proxys_sites(proxys,&aux_sites);
				lista_erase(&aux_sites);
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
				/* Worker estaba online y ha sufrido un fallo */
				printf("\tWorker %s paso de ONLINE a estado FALLANDO.\n",
					worker_get_name(worker));
				/* Generamos un listado con los sitios que atendia este worker */
				lista_copy(worker_get_sites(worker),&aux_sites);
				worker_purge(worker);
				assign_proxys_sites(proxys,&aux_sites);
				lista_erase(&aux_sites);
				/* El proceso normalice_sites se encargara de asignar
				   los sitios que se han eliminado del worker con falla */
			}
			cambio=1;
		}

		lista_next(workers);
	}
	printf("--FIN: Proceso check_workers\n");
	return cambio;
}

int normalice_sites(T_lista *sites, T_lista *workers,
		    T_lista *proxys, T_config *config){
	/* Recorre la lista de sitios */
	/* Aquellos sitios ONLINE donde la cantidad de workwes
	 * asignados no sea la adecuada, trata de corregirlo */

	/* Aquellos sitios OFFLINE que encuentra con workers
	 * asignados... los desasigna */

	/* Retorna 1 si al menos se a producido un cambio */

	static char status[30];
	T_site *site;
	T_lista candidates;
	int changed = 0;

	printf("\n----- NORMALICE ----\n");
	lista_first(sites);
	lista_init(&candidates,sizeof(T_worker));
	while(!lista_eol(sites)){
		site = lista_get(sites);

		itosstatus(site_get_status(site),status);
		printf("\tRevisamos el sitio %s - status: %s real %i: need:%i\n",
				site_get_name(site),status,site_get_real_size(site),site_get_size(site));

		if((site_get_real_size(site) < site_get_size(site)) && site_get_status(site) == S_ONLINE){
			/* Estan faltando workers */
			lista_erase(&candidates);
			if(select_workers(workers,&candidates,site)){
				printf("\tEstan faltando workers\n");
				changed |= assign_workers(&candidates,proxys,site,config);
			}
		} else if((site_get_real_size(site) > site_get_size(site)) ||
			   (site_get_real_size(site) && site_get_status(site) == S_OFFLINE)){
				/* Estan sobrando workers */
				printf("\tEstan sobrando workers\n");
				changed |= des_assign_workers(site,workers);
		} else {
			printf("Sitio: %s\t\tCORRECTO\n",site_get_name(site));
		}
		lista_next(sites);
	}
	lista_erase(&candidates);
	printf("\n----- FIN NORMALICE ----\n");
	return changed;
}

/********************************
 * 		MAIN		*
 ********************************/
void main(){

	int changed;
	int db_fail;
	char error[200];

	srand(time(NULL));

	/* Cargamos la configuracion */
	if(!config_load("controller.conf",&config))
		exit(1);

	printf("db_server : %s\n",config_db_server(&config));
	printf("db_user : %s\n",config_db_user(&config));
	printf("db_pass : %s\n",config_db_pass(&config));
	printf("db_name : %s\n",config_db_name(&config));
	printf("load_average : %i\n",config_load_average(&config));
	printf("site_average : %i\n",config_sites_average(&config));
	printf("log_file : %s\n",config_logs_file(&config));

	/* Levantamos el archivo de logs */
	if(!logs_init(&logs,config_logs_file(&config),config_logs_level(&config))){
		printf("Imposible levantar el archivo de logs\n");
		exit(1);
	}
	logs_write(&logs,L_INFO,"Start Controller","");

	/* Iniciamos estructuras */
	lista_init(&workers,sizeof(T_worker));
	lista_init(&sites,sizeof(T_site));
	lista_init(&proxys,sizeof(T_proxy));

	/* Conectamos contra la base de datos */
	db_init(&db,&config,&logs);
	if (!db_connect(&db)){
		logs_write(&logs,L_ERROR,"Error conexcion a la base de datos","");
		exit(1);
	}
	/* Cargamos los datos de la base de datos */
	printf("Controller: Cargando sitios\n");
	if(!db_load_sites(&db,&sites,error,&db_fail)){
		logs_write(&logs,L_ERROR,"Error al cargar los sitios desde la base de datos","");
		printf("Error al cargar los sitios\n");
		exit(1);
	}
	printf("Controller: Cargando workers\n");
	if(!db_load_workers(&db,&workers,error,&db_fail)){
		logs_write(&logs,L_ERROR,"Error al cargar los workers desde la base de datos","");
		printf("Error al cargar los workers\n");
		exit(1);
	}
	printf("Controller: Cargando proxys\n");
	if(!db_load_proxys(&db,&proxys,error,&db_fail)){
		logs_write(&logs,L_ERROR,"Error al cargar los proxys desde la base de datos","");
		printf("Error al cargar los proxys\n");
		exit(1);
	}

	/* Sincronizamos con la información en workers y proxys */
	printf("--INI Sincronizamos workers--\n");
	init_sync(&workers,&sites,&proxys,&logs);
	reload_services(&workers,&proxys);
	printf("--FIN Sincronizamos workers--\n");

	/* Iniciamos el server REST para la API */
	server_init(&server,&sites,&workers,&proxys,&db,&config,&logs);
	printf("El rest server posee puntero %p\n",server);

	/* Comenzamos el loop del controller */
	while(1){
		printf("Loop\n");
		//continue;
		changed = 0;
		server_lock(&server);
			/* Chequeo de workers */
			check_workers(&workers,&proxys,&config);
			if(changed)
				printf("@@@@@@@ check_workers CAMBIO\n");
			/* Chequeo de proxys */
			check_proxys(&proxys,&sites);	//No tiene sentido reiniciar todo
			
			/* Asignacion sitios a worker */
			changed |= normalice_sites(&sites, &workers, &proxys, &config);
			if(changed)
				printf("@@@@@@@ normalice_sites CAMBIO\n");
	
			/* Balanceamos workers */
			changed |= balance_workers(&workers,&proxys,&config);
			if(changed){ printf("@@@@@@@ balance_workers CAMBIO\n"); }
		server_unlock(&server);

		if(changed)
			reload_services(&workers,&proxys);

		/* Chequeo base de datos */
		check_db(&db);

		printf("Fin del bucle");
		sleep(10);
	}

	/* Finalizamos el hilo del server */

	/* Finalizamos la conexion a la base */
	db_close(&db);
}
