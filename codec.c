#include "codec.h"

void printbitchar(char a) {
	int i;

	for (i = 0; i < 8; i++) {
		printf("%d", !!((a << i) & 0x80));
	}
	printf("\n");
}

void u2char(unsigned int x, char *bytes){
	bytes[0] = (x >> 0) & 255;
	bytes[1] = (x >> 8) & 255;
	bytes[2] = (x >> 16) & 255;
	bytes[3] = (x >> 24);

//	printbitchar(bytes[0]);
//	printbitchar(bytes[1]);
//	printbitchar(bytes[2]);
//	printbitchar(bytes[3]);
}

void char2u(char *bytes, unsigned int *x){
	*x = (unsigned char)bytes[0];
	*x |=  (unsigned char)bytes[1] << 8;
	*x |=  (unsigned char)bytes[2] << 16;
	*x |=  (unsigned char)bytes[3] << 24;
	//printf("%u\n",x);
}
