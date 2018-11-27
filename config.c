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
	char buf[400];
	char atr[20];
	char val[200];

	fp = fopen(filename,"r");
	if(fp){
		while(fgets(buf, sizeof(buf), fp) != NULL){
			config_parce_line(&buf[0],&atr[0],&val[0]);
			if(0 == strcmp(&atr[0],"db_server")){strcpy(conf->db_server,&val[0]);}
			if(0 == strcmp(&atr[0],"db_name")){strcpy(conf->db_name,&val[0]);}
			if(0 == strcmp(&atr[0],"db_user")){strcpy(conf->db_user,&val[0]);}
			if(0 == strcmp(&atr[0],"db_pass")){strcpy(conf->db_pass,&val[0]);}
			if(0 == strcmp(&atr[0],"default_domain")){strcpy(conf->default_domain,&val[0]);}
			if(0 == strcmp(&atr[0],"load_average")){conf->load_average = atoi(&val[0]);}
			if(0 == strcmp(&atr[0],"sites_average")){conf->sites_average = atoi(&val[0]);}
			if(0 == strcmp(&atr[0],"log_file")){strcpy(conf->logs_file,&val[0]);}
			if(0 == strcmp(&atr[0],"default")){strcpy(conf->_default,&val[0]);}
			if(0 == strcmp(&atr[0],"log_level")){conf->logs_level = logs_str2level(&val[0]);}
			if(0 == strcmp(&atr[0],"webdir")){strcpy(conf->webdir,&val[0]);}
			if(0 == strcmp(&atr[0],"ftpuid")){strcpy(conf->ftpuid,&val[0]);}
		}
		fclose(fp);
		return 1;
	} else {
		return 0;
	}
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
char *config_default_domain(T_config *conf){
	return conf->default_domain;
}
int config_load_average(T_config *conf){
	return conf->load_average;
}
int config_sites_average(T_config *conf){
	return conf->sites_average;
}
char *config_logs_file(T_config *conf){
	return conf->logs_file;
}
char *config_default(T_config *conf){
	return conf->_default;
}
char *config_webdir(T_config *conf){
	return conf->webdir;
}
char *config_ftpuid(T_config *conf){
	return conf->ftpuid;
}
T_logs_level config_logs_level(T_config *conf){
	return conf->logs_level;
}
