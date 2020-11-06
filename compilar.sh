#!/bin/bash

ARG=''
if [ $1 = 'debug ' ]; then
	ARG='-g --enable-checking -Q -v -da -O0'
fi

rm controller
rm config.o
rm parce.o
rm db.o
rm structs.o
rm server.o
rm json.o
rm task.o
rm lista.o
rm dictionary.o
rm logs.o
rm valid.o
rm sock_connect.o
rm dim_string.o

gcc -c logs.c
gcc -c parce.c
gcc -c structs.c
gcc -c db.c
gcc -c config.c
gcc -c server.c
gcc -c json.c
gcc -c task.c
gcc -c lista.c
gcc -c dictionary.c
gcc -c valid.c
gcc -c sock_connect.c
gcc -c dim_string.c

gcc controller.c $ARG -lpthread -lmicrohttpd dim_string.o valid.o sock_connect.o dictionary.o logs.o lista.o config.o task.o json.o server.o parce.o db.o structs.o -L/usr/lib64/mysql/ -lmysqlclient -o controller
