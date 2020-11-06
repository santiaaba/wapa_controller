#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef _LOGS_H
#define _LOGS_H

#define	L_ERROR_DB	"Imposible acceder a la base de datos"

typedef enum {L_ERROR,L_WARNING,L_NOTICE,L_INFO,L_DEBUG} T_logs_level;

typedef struct logs{
	FILE *fd;
	char *filename;
	T_logs_level level;
} T_logs;

T_logs_level logs_str2level(char *string);
void logs_level2str(T_logs_level level,char *char_level);

int logs_init(T_logs *l, char *filename, T_logs_level level);
int logs_write(T_logs *l,T_logs_level a,char *title, char *message);
void logs_change_level(T_logs *l, T_logs_level level);
int logs_close(T_logs *l);
#endif
