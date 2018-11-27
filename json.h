#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structs.h"
#include "db.h"

#ifndef JSON_H
#define JSON_H
void json_servers(char **data, T_list_worker *workers,T_list_proxy *proxys, T_db *db);
int json_worker(char **data, T_worker *worker, T_db *db);
int json_proxy(char **data, T_proxy *proxy, T_db *db);
#endif
