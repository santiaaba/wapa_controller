/* La sintaxis dela rchivo de configuración debe ser
 * <variable>\t<valor>
 */

#include <stdio.h>

typedef struct config{
        char db_server[100];
        char db_name[20];
        char db_user[20];
        char db_pass[20];
} T_config;

int config_load(char *filename, T_config *conf);
