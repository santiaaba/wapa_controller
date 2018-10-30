#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structs.h"
#include "db.h"

#ifndef JSON_H
#define JSON_H
void json_sites(char **data, int *size, T_list_site *sites);
void json_site(char **data, int *size, T_site *site);
void json_servers(char **data, int *size, T_list_worker *workers,T_list_proxy *proxys, T_db *db);
int json_worker(char **data, int *size, T_worker *worker, T_db *db);
int json_proxy(char **data, int *size, T_proxy *proxy, T_db *db);
#endif
