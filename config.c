#include "config.h"

void config_parce_line(char *buf, char *atr, char *val){
	int i = 0;
	int j = 0;
	int size = strlen(buf);

	atr[0] = '\0';
	val[0] = '\0';
	// comentarios pasan de largo. Suponiendo que son el primer caracter
	if(buf[0] != '#'){
		// Buscamos el nombre del atributo
		while(i < size && buf[i] != '\t'){
			atr[i] = buf[i];
			i++;
		}
		atr[i] = '\0';
		while(i < size && (buf[i] == '\t' || buf[i] == ' ')){
			i++;
		}
		// Buscamos el valor del atributo
		while(i < size-1){
			val[j] = buf[i];
			i++;
			j++;
		}
		val[j] = '\0';
	}
}

int config_load(const char *filename, T_config *conf){
	/* Lee de un archivo de configuracion */
	FILE *fp;
	char buf[200];
	char atr[20];
	char val[20];

	fp = fopen(filename,"r");
	while(fgets(buf, sizeof(buf), fp) != NULL){
		config_parce_line(&buf[0],&atr[0],&val[0]);
		if(0 == strcmp(&atr[0],"db_server")){strcpy(conf->db_server,&val[0]);}
		if(0 == strcmp(&atr[0],"db_name")){strcpy(conf->db_name,&val[0]);}
		if(0 == strcmp(&atr[0],"db_user")){strcpy(conf->db_user,&val[0]);}
		if(0 == strcmp(&atr[0],"db_pass")){strcpy(conf->db_pass,&val[0]);}
	}
	fclose(fp);
}

char *config_db_server(T_config *conf){
	return conf->db_server;
}
char *config_db_name(T_config *conf){
	return conf->db_name;
}
char *config_db_user(T_config *conf){
	return conf->db_user;
}
char *config_db_pass(T_config *conf){
	return conf->db_pass;
}
