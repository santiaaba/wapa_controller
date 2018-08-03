#include "parce.h"

void parce_data(char *buffer, int *i, char *value){
	/* Dado un comando recibido desde el servidor con formato
 	 * de campos separados por |, retorna el valor a partir
	 * de la posición dada hasta el proximo | o fin del string.
	 * Retorna ademas la posición fin en i. los datos comienzan
	 * en la posicion 3*/

	int j = 0;
	int largo;

	largo = strlen(buffer);
	printf("Parceando %s\n",buffer);
	while(*i < largo && buffer[*i] != '|' && buffer[*i] != '\0'){
		printf("la i es: %i\n",*i);
		printf("%c\n",buffer[*i]);
		value[j] = buffer[*i];
		j++; (*i)++;
	}
	value[j]='\0';
	/* Adelantamos la i una posicion ya que se
	 * encuentra parada en el caracter "|" posiblemente */
	(*i)++;
	printf("valor: %s\n",value);
}
