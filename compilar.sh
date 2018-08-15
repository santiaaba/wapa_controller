#!/bin/bash
rm config.o
rm parce.o
rm db.o
rm structs.o
rm rest_server.o
rm json.o
rm job.o

gcc -c parce.c
gcc -c structs.c
gcc -c db.c
gcc -c config.c
gcc -c rest_server.c
gcc -c json.c
gcc -c job.c

gcc controller.c -lpthread -lmicrohttpd config.o job.o json.o rest_server.o parce.o db.o structs.o -L/usr/lib64/mysql/ -lmysqlclient -o controller
