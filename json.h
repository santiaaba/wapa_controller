#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structs.h"

#ifndef JSON_H
#define JSON_H
void json_sites(char **data, int *size, T_list_site *sites);
void json_site(char **data, int *size, T_site *site);
#endif