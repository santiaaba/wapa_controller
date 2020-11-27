#include "dim_string.h"

void dim_init(char **s1){
	*s1=(char *)realloc(*s1,1);
	*s1[0] = '\0';
}

void dim_copy(char **s1, char *s2){
	//printf("dim aca: %p\n",*s1);
	*s1=(char *)realloc(*s1, strlen(s2) + 1);
	//printf("dim aca 2\n");
	strcpy(*s1,s2);
	//printf("dim aca 3\n");
}

void dim_end(char **s1,char c){
	/* Reemplaza el ultimo caracter
 	   con el indicado por parametro */

	/*
	char *aux=NULL;
	int l;
	*/

	printf("ACAAA: %s - %c\n",*s1,(*s1)[strlen(*s1)-1]);
	(*s1)[strlen(*s1)-1] = c;
	printf("PASO CAAA\n");
	/*
	l = strlen(*s1);
	printf("dim_trim 1: %s - size: %i\n",*s1,l);
	aux=(char *)malloc(l);
	strncpy(aux,*s1,l-1);
	strcpy(*s1,aux);
	free(aux);
	*/
}

void dim_concat(char **s1, char *s2){
	/* Concatena s1 con s2 retornando el
	 * resultado en s1 dimencionando la
	 * memoria si es que no entra */
	//printf("Entro concat\n");
	//printf("s1 length: %i\n", strlen(*s1));
	//printf("s2 length: %i\n", strlen(s2));
	int tam = strlen(*s1) + strlen(s2) + 1;
	//printf("dim_concat: '%s' + '%s'\n",*s1,s2);
	//printf("dim_concat tam: %i para %p\n",tam,*s1);
	*s1=(char *)realloc(*s1,tam);
	//printf("dim_concat: CONCATENAMOS\n");
	strcat(*s1,s2);
	//printf("dim_concat resultado: '%s'\n",*s1);
}
