#include "valid.h"

int valid_domain(char *s){
	/* Indica si el string es un dominio valido */
	int size;
	int i=0;
	int ok=1;

	printf("Validando %s\n",s);
	if(s == NULL){
		printf("Invalido s null\n");
		return 0;
	}
	size =strlen(s);
	if(size < DOMAIN_MIN_LEN || size > DOMAIN_MAX_LEN){
		printf("Invalido exede\n");
		return 0;
	}
	if(s[i] == '.'){
		printf("Invalido. Es '.'\n");
		return 0;
	}

	while(ok && i<size){
		if(s[i] == '.'){
			ok = valid_domain(s+i+1);
			break;
		}
		ok = (47 < (int)s[i] && (int)s[i] < 58) ||	//0..9
		     (64 < (int)s[i] && (int)s[i] < 91) ||	//A..Z
		     (96 < (int)s[i] && (int)s[i] < 123) ||	//a..z
		     ((int)s[i] == 95);
		i++;
	}
	return ok;
}

int valid_user_name(char *s){

	int ok=1;
	int size;
	int i;

	if(s == NULL)
		return 0;
	size = strlen(s);
	if(size < USER_MIN_LEN || size > USER_MAX_LEN)
		return 0;
	i=0;
	while(ok && i<size){
		ok = ((47 < (int)s[i] && (int)s[i] < 58) ||	//0..9
		      (64 < (int)s[i] && (int)s[i] < 91) ||	//A..Z
		      (96 < (int)s[i] && (int)s[i] < 123) ||	//a..z
		      ((int)s[i] == 45) ||			//-
		      ((int)s[i] == 95));			//_
		i++;
	}
	return ok;
}
int valid_passwd(char *s){

	int ok=1;
	int size;
	int i;

	if(s == NULL)
		return 0;
	size = strlen(s);
	if(size < PASS_MIN_LEN || size > PASS_MAX_LEN)
		return 0;
	i=0;
	while(ok && i<size){
		ok = (47 < (int)s[i] && (int)s[i] < 58) ||
		     (64 < (int)s[i] && (int)s[i] < 91) ||
		     (96 < (int)s[i] && (int)s[i] < 123) ||
		     ((int)s[i] == 45) ||
                     ((int)s[i] == 95) ||
		     ((int)s[i] == 33) ||
		     ((int)s[i] == 46);
		i++;
	}
	return ok;
}
int valid_email(char *s){

	int ok=1;
	int size;
	int i;

	if(s == NULL)
		return 0;
	size = strlen(s);
	if(size < EMAIL_MIN_LEN || size > EMAIL_MAX_LEN )
		return 0;
	i=0;
	// Verificamos si antes del arroba es valido
	while(ok && i<size && s[i] != '@'){
		ok = (47 < (int)s[i] && (int)s[i] < 58) ||
		     (64 < (int)s[i] && (int)s[i] < 91) ||
		     (96 < (int)s[i] && (int)s[i] < 123) ||
		     ((int)s[i] == 95) ||
		     ((int)s[i] == 46);
		i++;
	}
	if(ok)
		ok &= (s[i] == '@');
	// Verificamos el dominio
	if(ok)
		ok = valid_domain(s+i+1);
	return ok;
}

int valid_id(char *s){

	int ok=1;
	int size;
	int i;

	printf("Validando id: %s\n",s);
	if(s == NULL)
		return 0;
	size = strlen(s);
	if(size < ID_MIN_LEN || size > ID_MAX_LEN )
		return 0;
	if((int)s[0] == 48)	//El primer digito no puede ser 0
		return 0;
	i=1;
	while(ok && i<size){
		ok = ((47 < (int)s[i] && (int)s[i] < 58));
		i++;
	}
	return ok;

}

int valid_size(char *s){

	int ok=1;
	int size;
	int i;

	if(s == NULL)
		return 0;
	size = strlen(s);
	if(size < SIZE_MIN_LEN || size > SIZE_MAX_LEN )
		return 0;
	if((int)s[0] == 48)
		return 0;
	i=1;
	while(ok && i<size){
		ok = ((47 < (int)s[i] && (int)s[i] < 58));
		i++;
	}
	return ok;
}

int valid_namespace_name(char *s){
	/* IMPLEMENTAR */
	return 1;
}

int valid_site_name(char *s){
	/* IMPLEMENTAR */
	return 1;
}

int valid_site_url(char *s){
	/* IMPLEMENTAR */
	return 1;
}

int valid_ftp_name(char *s){
	/* IMPLEMENTAR */
	return 1;
}
