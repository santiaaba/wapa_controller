/* La sintaxis dela rchivo de configuración debe ser
 * <variable>\t<valor>
 */

#include <stdio.h>
#include <string.h>
#include "logs.h"

#ifndef CONFIG_H
#define CONFIG_H

typedef struct config{
        char db_server[100];
        char db_name[20];
        char db_user[20];
        char db_pass[20];
        char ftpuid[10];		//uid del usuario ftp
        char logs_file[100];
	char _default[200];		// path absoluto archivo default.html
        T_logs_level logs_level;
	char default_domain[100];	// Dominio por defecto
	char webdir[100];		/* Directorio absoluto a partir del
					   cual se generan los hash de los sitios */
	int load_average;
	int sites_average;
} T_config;

int config_load(const char *filename, T_config *conf);
char *config_db_server(T_config *conf);
char *config_db_name(T_config *conf);
char *config_db_user(T_config *conf);
char *config_db_pass(T_config *conf);
char *config_ftpuid(T_config *conf);
char *config_logs_file(T_config *conf);
char *config_default(T_config *conf);
char *config_default_domain(T_config *conf);
char *config_webdir(T_config *conf);
T_logs_level config_logs_level(T_config *conf);
int config_load_average(T_config *conf);
int config_sites_average(T_config *conf);

#endif
