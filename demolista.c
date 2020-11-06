#include <stdio.h>
#include "lista.h"

int buscar_numero(void *a){
	int b;
	return *(int*)a;
}

void main(){
	T_list lista1;
	long int *aux;
	int i;

	printf("Aca llegamos\n");
	list_init(&lista1,sizeof(long int));
	aux = (long int *)malloc(sizeof(long int));
	for(i=0;i<10;i++){
		*aux = i;
		printf("Agregamos: %i\n",*aux);
		list_add(&lista1,aux);
	}
	list_first(&lista1);
	for(i=0;i<list_size(&lista1);i++){
		aux = list_get(&lista1);
		printf("Recuperamos: %i\n",*aux);
		list_next(&lista1);
	}

	/* Buscando uno en particular */
	aux = list_find_id(&lista1,buscar_numero,5);
	printf("Recuperamos con Find: %i\n",*aux);
}
