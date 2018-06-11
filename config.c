#include "config.h"

int config_load(char *filename, T_config *conf){
	/* Lee de un archivo de configuracion */
	FILE *fp;
	char buf[200];

	fp = fopen(filename,"r");
	while(fgets(buf, sizeof(buf), fp) != NULL){
		printf("%s\n",buf);
	}
	fclose(fp);
}
