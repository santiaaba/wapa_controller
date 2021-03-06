#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef DIM_STRING_H
#define DIM_STRING_H

void dim_new(char **s1);
void dim_init(char **s1);
void dim_copy(char **s1, char *s2);
void dim_end(char **s1, char c);
void dim_to_json(char *s1, char **s2);
void dim_concat(char **s1, char *s2);

#endif
