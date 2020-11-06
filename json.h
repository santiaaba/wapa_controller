#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structs.h"
#include "db.h"
#include "lista.h"

#ifndef JSON_H
#define JSON_H
void json_servers(char **data, T_lista *workers,T_lista *proxys, T_db *db);
int json_worker(char **data, T_worker *worker, T_db *db);
int json_proxy(char **data, T_proxy *proxy, T_db *db);

void json_mysql_result(MYSQL_RES *result, char **message);
void json_mysql_result_row(MYSQL_ROW *row, char *col_names[50], int cant_cols, char **message);
#endif
