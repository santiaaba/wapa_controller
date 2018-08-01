/* La sintaxis dela rchivo de configuración debe ser
 * <variable>\t<valor>
 */

#include <stdio.h>
#include <string.h>

#ifndef CONFIG_H
#define CONFIG_H

typedef struct config{
        char db_server[100];
        char db_name[20];
        char db_user[20];
        char db_pass[20];
} T_config;

int config_load(const char *filename, T_config *conf);
char *config_db_server(T_config *conf);
char *config_db_name(T_config *conf);
char *config_db_user(T_config *conf);
char *config_db_pass(T_config *conf);

#endif
