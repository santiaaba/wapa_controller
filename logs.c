#include "logs.h"

void logs_level2str(T_logs_level level,char *char_level){
	switch(level){
		case L_ERROR: strcpy(char_level,"ERROR"); break;
		case L_WARNING: strcpy(char_level,"WARNING"); break;
		case L_NOTICE: strcpy(char_level,"NOTICE"); break;
		case L_INFO: strcpy(char_level,"INFO"); break;
		case L_DEBUG: strcpy(char_level,"DEBUG"); break;
	}
}

T_logs_level logs_str2level(char *string){
	if(strcmp(string,"ERROR")==0) return L_ERROR;
	if(strcmp(string,"WARNING")==0) return L_WARNING;
	if(strcmp(string,"NOTICE")==0) return L_NOTICE;
	if(strcmp(string,"INFO")==0) return L_INFO;
	if(strcmp(string,"DEBUG")==0) return L_DEBUG;
}

int logs_init(T_logs *l, char *filename, T_logs_level level){
	l->level = level;
	l->filename = (char*)malloc(strlen(filename)+1);
	strcpy(l->filename,filename);
	l->fd = fopen(filename,"a");
	if(!(l->fd)){
		printf("Imposible habrir el archivo de logs %s\n",filename);
		return 0;
	}
	return 1;
}

int logs_write(T_logs *l,T_logs_level level,char *title, char *message){
	char char_level[20];
	char fecha[30];
	time_t timer;
	struct tm* tm_info;

	if(level <= l->level)
		time(&timer);
		tm_info = localtime(&timer);
		strftime(fecha, 26, "%Y-%m-%d %H:%M:%S", tm_info);
		logs_level2str(level,char_level);
		fprintf(l->fd,"%s:%s:%s - %s\n",fecha,char_level,title, message);
}

void logs_change_level(T_logs *l, T_logs_level level){
	l->level = level;
}

int logs_close(T_logs *l){
	fclose(l->fd);
}
