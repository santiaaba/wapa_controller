#include <stdio.h>
#include <stdlib.h>

int memoria(char **palabra){

	*palabra = (char *)realloc(*palabra,100);
}

int main(){
	char *palabra = NULL;
	char *palabra2 = NULL;

	printf("paso 1\n");
	memoria(&palabra);
	printf("paso 2\n");
	palabra = (char *)realloc(palabra,100);
	printf("paso 3\n");
}
