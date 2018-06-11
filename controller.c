#include "structs.h"
#include "config.h"

/********************************
 * 	Variables GLOBALES	*
 ********************************/
T_config config;

/********************************
 * 	FUNCIONES		*
 ********************************/

void db_load_sites(){
}

void db_load_workers(){
}

void db_load_proxys(){
}

/********************************
 * 		MAIN		*
 ********************************/
void main(){

	/* Cargamos la configuracion */
	config_load("controller.conf",&config);
}
